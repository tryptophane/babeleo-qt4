/*
 * Babeleo - KDE Plasma 6 Plasmoid
 * Main file: Defines the visual appearance of the applet.
 *
 * In Plasma 6 the UI is written entirely in QML.
 * The C++ part (babeleo.cpp) handles only business logic.
 *
 * Important concepts:
 * - PlasmoidItem: root element of every Plasma 6 applet
 * - compactRepresentation: what is shown in the panel bar (small icon)
 * - fullRepresentation: what is shown in the popup (text input for manual search)
 * - Plasmoid: the "attached object" - provides access to C++ properties (Q_PROPERTY)
 *             and calls C++ methods (Q_INVOKABLE)
 */

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid        // PlasmoidItem, Plasmoid attached object
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    // Prevent Plasma from automatically toggling the popup when the global
    // shortcut ("Activate widget as if clicked") fires. Without this, Plasma
    // opens the fullRepresentation popup AND our onActivated handler also runs,
    // causing the popup AND the browser to open simultaneously. The popup
    // opening also steals focus, preventing the browser from coming to the front.
    // We handle activation exclusively in the Connections block below.
    activationTogglesExpanded: false

    // Set to true during onActivated handling to block Plasma's setExpanded(true),
    // which is called AFTER the activated() signal is emitted (not before).
    property bool blockExpand: false

    onExpandedChanged: {
        if (blockExpand && expanded) {
            // Cannot set expanded synchronously here - QML has a reentrancy guard
            // that silently ignores changes to expanded inside onExpandedChanged.
            // Qt.callLater defers the close until after this handler returns.
            Qt.callLater(function() { root.expanded = false })
        }
    }

    // =========================================================================
    // Compact Representation: The icon in the panel bar
    // =========================================================================
    compactRepresentation: Item {
        id: compactItem

        // Kirigami.Icon displays the icon of the currently selected dictionary.
        // Plasmoid.icon is a Q_PROPERTY in our C++ class Babeleo.
        Kirigami.Icon {
            id: appletIcon
            anchors.fill: parent
            source: Plasmoid.icon
            // "active" makes the icon slightly brighter on hover
            active: mouseArea.containsMouse
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton

            onClicked: function(mouse) {
                // Left click: translate clipboard content directly.
                // root.plasmoid is the actual C++ Babeleo object (QObject* from applet()).
                // IMPORTANT: Plasmoid (attachment) only knows the Plasma::Applet API.
                //            root.plasmoid (property of PlasmoidItem) returns our
                //            Babeleo instance with all Q_INVOKABLE methods.
                root.plasmoid.browseWithClipboard()
            }
        }

        // Tooltip shows the current search engine.
        // PlasmaComponents3.ToolTip is displayed when hovering over the icon.
        PlasmaComponents3.ToolTip {
            text: i18n("Translate clipboard content\nCurrent engine: %1", root.plasmoid.currentEngine)
        }
    }

    // =========================================================================
    // Full Representation: The popup for manual search
    // (opened via context menu or global keyboard shortcut)
    // =========================================================================
    fullRepresentation: PlasmaExtras.Representation {
        id: fullRep

        Layout.minimumWidth: Kirigami.Units.gridUnit * 20
        // Constrain popup height to exactly fit the content.
        // Without this, PlasmaExtras.Representation grows to a large default size.
        // contentLayout.implicitHeight gives us the exact height of TextField + buttons.
        Layout.preferredHeight: contentLayout.implicitHeight + Kirigami.Units.smallSpacing * 4
        Layout.maximumHeight: Layout.preferredHeight

        // Header with icon and name of the current search engine
        header: PlasmaExtras.PlasmoidHeading {
            RowLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: Plasmoid.icon
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                PlasmaComponents3.Label {
                    text: root.plasmoid.currentEngine
                    Layout.fillWidth: true
                    font.bold: true
                }
            }
        }

        ColumnLayout {
            id: contentLayout
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.TextField {
                id: queryField
                Layout.fillWidth: true
                placeholderText: i18n("Enter search term…")
                // Automatically focus when the popup opens
                focus: true

                // Enter key starts the search
                Keys.onReturnPressed: startSearch()
                Keys.onEnterPressed: startSearch()
                // Escape closes the popup
                Keys.onEscapePressed: root.expanded = false
            }

            RowLayout {
                Layout.fillWidth: true

                // Spacer pushes buttons to the right
                Item { Layout.fillWidth: true }

                PlasmaComponents3.Button {
                    icon.name: "dialog-cancel"
                    text: i18n("Cancel")
                    onClicked: root.expanded = false
                }

                PlasmaComponents3.Button {
                    icon.name: "edit-find"
                    text: i18n("Search")
                    onClicked: startSearch()
                }
            }
        }

        // When the popup becomes visible, set focus to the text field
        onVisibleChanged: {
            if (visible) {
                queryField.text = ""
                queryField.forceActiveFocus()
            }
        }

        function startSearch() {
            if (queryField.text.trim().length > 0) {
                root.plasmoid.browseWithText(queryField.text)
                root.expanded = false
            }
        }
    }

    // =========================================================================
    // Receiving signals - two separate Connections blocks are needed:
    //
    // 1. "Plasmoid" (attachment object, type PlasmaQuick::PlasmoidAttached):
    //    Only knows the Plasma::Applet API. "activated" is defined there
    //    and fires when the global keyboard shortcut is triggered.
    //
    // 2. "root.plasmoid" (Q_PROPERTY on PlasmoidItem, returns QObject*):
    //    Is our actual Babeleo object at runtime. QML uses the dynamic
    //    type here and finds our own Q_SIGNALs such as
    //    "requestTogglePopup".
    //
    // Java analogy: Plasmoid = interface, root.plasmoid = concrete class.
    // =========================================================================
    // "activated" fires when the global shortcut "Activate widget as if clicked"
    // is triggered (configured in Plasma widget settings → Keyboard Shortcuts).
    //
    // Timing: Plasma emits activated() FIRST, then calls setExpanded(true) after.
    // So when our handler runs, expanded is still false - we cannot block it here.
    // Instead, blockExpand = true causes onExpandedChanged to immediately close
    // the popup when Plasma's setExpanded(true) arrives after our handler.
    // Qt.callLater resets blockExpand after all pending events are processed,
    // so manual expansion via context menu still works normally afterwards.
    Connections {
        target: Plasmoid
        function onActivated() {
            root.blockExpand = true
            root.plasmoid.browseWithClipboard()
            Qt.callLater(function() { root.blockExpand = false })
        }
    }

    Connections {
        target: root.plasmoid
        function onRequestTogglePopup() {
            root.expanded = !root.expanded
        }
    }
}
