/*
 * Babeleo - Search Engines configuration page
 *
 * Appears as "Search Engines" in the standard Plasma settings dialog sidebar.
 *
 * Architecture (local-model pattern):
 *   All edits go into the local ListModel only - C++ is never touched until
 *   the user explicitly clicks Apply or OK (saveConfig() is called by Plasma).
 *   This means Cancel correctly discards everything without any cleanup.
 *
 * Data flow:
 *   Load:        Plasmoid.self.engineList() → engineModel + originalEngineNames
 *   Edit form:   form fields → engineModel (via commitCurrentToModel())
 *   Switch item: commitCurrentToModel(), then loadFormFromModel(newIndex)
 *   Add/Delete:  engineModel only (no C++ calls)
 *   Fetch icon:  Plasmoid.self.fetchIcon() → async signal → update engineModel
 *   Apply/OK:    saveConfig() → compare model with originalEngineNames →
 *                deleteEngine() for removed, saveEngine() for all current →
 *                rebuildAfterConfigChange()
 *
 * The "loading" flag suppresses onTextChanged / onCheckedChanged handlers
 * while we populate form fields programmatically, preventing false
 * configurationChanged() signals that would incorrectly enable the Apply button.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QC
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

Item {
    id: root

    // Standard Plasma config page interface
    signal configurationChanged
    property bool localChanges: false

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------

    // Suppresses form field change handlers during programmatic population.
    // Like a "batch update" flag - prevents false configurationChanged signals.
    property bool loading: false

    // Index of the engine currently shown in the form
    property int currentIndex: -1

    // Original engine names from C++ at load time, used in saveConfig() to
    // detect which engines were deleted (present in original but not in model).
    property var originalEngineNames: []

    ListModel {
        id: engineModel
    }

    // -------------------------------------------------------------------------
    // Data functions
    // -------------------------------------------------------------------------

    // Populate the local model from C++. Called once on load.
    function loadAllEngines() {
        engineModel.clear()
        const list = Plasmoid.self.engineList()
        originalEngineNames = list.map(e => e.name)
        for (const eng of list) {
            engineModel.append({
                name:     eng.name,
                url:      eng.url,
                icon:     eng.icon,
                position: eng.position,
                hidden:   eng.hidden
            })
        }
    }

    // Write current form values into the local model (no C++ calls).
    // Called before switching to another engine or before saveConfig().
    function commitCurrentToModel() {
        if (currentIndex < 0 || currentIndex >= engineModel.count) return
        const newName = nameField.text.trim()
        if (!newName) return
        engineModel.setProperty(currentIndex, "name",     newName)
        engineModel.setProperty(currentIndex, "url",      urlField.text)
        engineModel.setProperty(currentIndex, "icon",     iconField.text)
        engineModel.setProperty(currentIndex, "position", mainMenuRadio.checked ? "0" : "1")
        engineModel.setProperty(currentIndex, "hidden",   hideBox.checked)
        // If the name changed, currentIndex still points to the same slot,
        // but the displayed name in the list updates via the model binding.
    }

    // Load form fields from the local model at the given index.
    // Sets loading = true so form field handlers don't fire configurationChanged.
    function loadFormFromModel(index) {
        if (index < 0 || index >= engineModel.count) return
        loading = true
        const eng = engineModel.get(index)
        nameField.text        = eng.name
        urlField.text         = eng.url
        iconField.text        = eng.icon
        hideBox.checked       = eng.hidden
        mainMenuRadio.checked = (eng.position === "0")
        otherRadio.checked    = (eng.position !== "0")
        loading = false
        currentIndex = index
    }

    // Switch to a different engine in the list, committing any pending changes first.
    function switchToEngine(index) {
        if (index === currentIndex) return
        commitCurrentToModel()
        engineList.currentIndex = index
        loadFormFromModel(index)
        // Don't reset localChanges - the user may have already made changes
        // to other engines. localChanges tracks ANY pending change in the session.
    }

    // Called by Plasma when the user clicks Apply or OK.
    // Syncs the complete local model to C++.
    function saveConfig() {
        // Commit whatever is currently in the form to the model
        commitCurrentToModel()

        // Find engines that existed in C++ but are no longer in the model.
        // (user deleted them during this config session)
        const newNames = []
        for (let i = 0; i < engineModel.count; i++) {
            newNames.push(engineModel.get(i).name)
        }
        for (const origName of originalEngineNames) {
            if (!newNames.includes(origName)) {
                Plasmoid.self.deleteEngine(origName)
            }
        }

        // Save all engines from the local model to C++.
        // For each engine: use the ORIGINAL name as oldName if it existed,
        // or "" if it is a newly added engine.
        // saveEngine("", newName, ...) creates a new entry.
        // saveEngine(origName, newName, ...) replaces the existing entry.
        for (let i = 0; i < engineModel.count; i++) {
            const eng = engineModel.get(i)
            // If the engine's CURRENT name matches an original name, use it as oldName.
            // Renames: the old name was deleted above (not in newNames), new name is created here.
            const oldName = originalEngineNames.includes(eng.name) ? eng.name : ""
            Plasmoid.self.saveEngine(oldName, eng.name, eng.url, eng.icon,
                                     eng.position, eng.hidden)
        }

        // Rebuild the applet's context menu and emit signals
        Plasmoid.self.rebuildAfterConfigChange()

        // Update our baseline for the next Apply click
        originalEngineNames = newNames
        localChanges = false
    }

    Component.onCompleted: {
        loadAllEngines()
        if (engineModel.count > 0) {
            engineList.currentIndex = 0
            loadFormFromModel(0)
        }
    }

    // Receive async fetchIcon() result
    Connections {
        target: Plasmoid.self
        function onIconFetched(engineName, iconPath) {
            fetchButton.enabled = true
            fetchButton.text = i18nd("plasma_applet_babeleo","Fetch Icon")
            if (!iconPath) return

            // Update the local model entry
            for (let i = 0; i < engineModel.count; i++) {
                if (engineModel.get(i).name === engineName) {
                    engineModel.setProperty(i, "icon", iconPath)
                    break
                }
            }
            // Update form field if this engine is currently displayed (suppressed)
            if (currentIndex >= 0 && engineModel.get(currentIndex).name === engineName) {
                loading = true
                iconField.text = iconPath
                loading = false
            }
            localChanges = true
            root.configurationChanged()
        }
    }

    // -------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        // Left panel: engine list + Add/Delete buttons
        ColumnLayout {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 12
            Layout.minimumWidth: Kirigami.Units.gridUnit * 12
            Layout.maximumWidth: Kirigami.Units.gridUnit * 12
            Layout.fillHeight: true
            spacing: Kirigami.Units.smallSpacing

            PC3.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: engineList
                    model: engineModel
                    clip: true

                    highlight: Rectangle {
                        color: Kirigami.Theme.highlightColor
                        radius: 3
                        opacity: 0.6
                    }
                    highlightMoveDuration: 0

                    delegate: QC.ItemDelegate {
                        width: ListView.view.width
                        highlighted: ListView.isCurrentItem

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                source: model.icon
                                implicitWidth:  Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                opacity: model.hidden ? 0.4 : 1.0
                            }
                            PC3.Label {
                                text: model.name
                                color: model.hidden
                                    ? Kirigami.Theme.disabledTextColor
                                    : Kirigami.Theme.textColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        onClicked: switchToEngine(index)
                    }
                }
            }

            // Add / Delete buttons — equal width, with margins
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.smallSpacing * 2
                Layout.rightMargin: Kirigami.Units.smallSpacing * 2
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                PC3.Button {
                    icon.name: "list-add"
                    text: i18nd("plasma_applet_babeleo","Add")
                    Layout.fillWidth: true
                    onClicked: {
                        commitCurrentToModel()
                        // Add a new entry to the local model only (no C++ call)
                        const newName = i18nd("plasma_applet_babeleo","New Engine")
                        engineModel.append({
                            name: newName, url: "https://",
                            icon: "babelfishleo", position: "1", hidden: false
                        })
                        const newIdx = engineModel.count - 1
                        engineList.currentIndex = newIdx
                        loadFormFromModel(newIdx)
                        localChanges = true
                        root.configurationChanged()
                    }
                }

                PC3.Button {
                    icon.name: "list-remove"
                    text: i18nd("plasma_applet_babeleo","Delete")
                    Layout.fillWidth: true
                    enabled: currentIndex >= 0 && engineModel.count > 0
                    onClicked: {
                        if (currentIndex < 0) return
                        // Remove from local model only (no C++ call)
                        engineModel.remove(currentIndex)
                        const newIdx = Math.max(0, Math.min(currentIndex, engineModel.count - 1))
                        if (engineModel.count > 0) {
                            engineList.currentIndex = newIdx
                            loadFormFromModel(newIdx)
                        } else {
                            currentIndex = -1
                        }
                        localChanges = true
                        root.configurationChanged()
                    }
                }
            }
        }

        // Visual divider
        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: Kirigami.Theme.separatorColor
        }

        // Right panel: edit form
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            enabled: currentIndex >= 0 && engineModel.count > 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                PC3.Label { text: i18nd("plasma_applet_babeleo","Name:") }
                PC3.TextField {
                    id: nameField
                    Layout.fillWidth: true
                    onTextChanged: if (!loading) { localChanges = true; root.configurationChanged() }
                }

                Item { Layout.fillWidth: true; implicitHeight: Kirigami.Units.largeSpacing * 2}

                PC3.Label { text: i18nd("plasma_applet_babeleo","URL (%s = search term):") }
                PC3.TextField {
                    id: urlField
                    Layout.fillWidth: true
                    onTextChanged: if (!loading) { localChanges = true; root.configurationChanged() }
                }

                Item { Layout.fillWidth: true; implicitHeight: Kirigami.Units.largeSpacing * 2 }

                PC3.Label { text: i18nd("plasma_applet_babeleo","Icon:") }
                RowLayout {
                    Layout.fillWidth: true
                    PC3.TextField {
                        id: iconField
                        Layout.fillWidth: true
                        placeholderText: i18nd("plasma_applet_babeleo","Theme name or file path")
                        onTextChanged: if (!loading) { localChanges = true; root.configurationChanged() }
                    }
                    PC3.Button {
                        id: fetchButton
                        text: i18nd("plasma_applet_babeleo","Fetch Icon")
                        icon.name: "download"
                        onClicked: {
                            commitCurrentToModel()
                            fetchButton.enabled = false
                            fetchButton.text = i18nd("plasma_applet_babeleo","Downloading\u2026")
                            Plasmoid.self.fetchIcon(nameField.text, urlField.text)
                        }
                    }
                }

                Item { Layout.fillWidth: true; implicitHeight: Kirigami.Units.largeSpacing * 2 }

                PC3.Label { text: i18nd("plasma_applet_babeleo","Visibility:") }
                PC3.CheckBox {
                    id: hideBox
                    text: i18nd("plasma_applet_babeleo","Hide this search engine")
                    onCheckedChanged: if (!loading) { localChanges = true; root.configurationChanged() }
                }

                Item { Layout.fillWidth: true; implicitHeight: Kirigami.Units.largeSpacing * 2 }

                PC3.Label { text: i18nd("plasma_applet_babeleo","Position:") }
                RowLayout {
                    PC3.RadioButton {
                        id: mainMenuRadio
                        text: i18nd("plasma_applet_babeleo","Main menu")
                        onCheckedChanged: if (!loading && checked) {
                            localChanges = true; root.configurationChanged()
                        }
                    }
                    PC3.RadioButton {
                        id: otherRadio
                        Layout.leftMargin: Kirigami.Units.largeSpacing * 2
                        text: i18nd("plasma_applet_babeleo","More search engines submenu")
                        onCheckedChanged: if (!loading && checked) {
                            localChanges = true; root.configurationChanged()
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }  // push form to top
        }
    }
}
