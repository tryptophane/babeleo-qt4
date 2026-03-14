/***********************************************************************************
* Babeleo: Plasmoid for translating clipboard content at leo.org.
* Copyright (C) 2009 Pascal Pollet <pascal@bongosoft.de>
*
* Plasma 6 / KDE Frameworks 6 port.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
***********************************************************************************/

#pragma once

#include "babelengine.h"

#include <Plasma/Applet>             // Base class for all Plasma applets
#include <KConfigGroup>              // KDE configuration system

#include <QAction>
#include <QActionGroup>
#include <QHash>
#include <QStringList>
#include <QVariantList>

/**
 * Babeleo - Plasma 6 applet for quick dictionary lookups.
 *
 * Architecture in Plasma 6:
 * - This C++ class: BUSINESS LOGIC (managing search engines, opening URLs, config)
 * - main.qml:       UI (icon in the panel, popup dialog)
 *
 * Communication C++ → QML:
 * - Q_PROPERTY: QML reads values via "Plasmoid.propertyName"
 * - Q_SIGNAL:   QML receives events via "Connections { target: Plasmoid }"
 *
 * Communication QML → C++:
 * - Q_INVOKABLE: QML calls methods via "Plasmoid.methodName()"
 */
class Babeleo : public Plasma::Applet
{
    Q_OBJECT

    // Q_PROPERTY makes C++ members accessible to QML.
    // READ: getter method; NOTIFY: signal on change (important for QML bindings!)
    Q_PROPERTY(QString currentEngine READ currentEngine NOTIFY currentEngineChanged)

    /** Exposes this applet instance to QML config pages via Plasmoid.self.
     *  QML uses dynamic dispatch on the returned QObject*, giving access
     *  to all Q_INVOKABLE methods of Babeleo (not just Plasma::Applet).
     *  Used in configSearchEngines.qml: Plasmoid.self.engineList() etc.
     */
    Q_PROPERTY(QObject *self READ self CONSTANT)

public:
    // Plasma 6 constructor signature: KPluginMetaData instead of QVariantList as second parameter
    Babeleo(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~Babeleo() override;

    // init() is called by Plasma once the applet has been initialized.
    // Comparable to a Spring @PostConstruct or Angular ngOnInit.
    void init() override;

    // configChanged() is called by Plasma when config changes externally.
    void configChanged() override;

    // contextualActions() returns the entries for the right-click context menu.
    QList<QAction *> contextualActions() override;

    // Getter for the Q_PROPERTY currentEngine
    QString currentEngine() const;

    // Getter for the Q_PROPERTY self - returns this instance as QObject* for QML dynamic dispatch
    QObject *self() { return this; }

    // Q_INVOKABLE: These methods can be called directly from QML
    // via "Plasmoid.methodName()" or "Plasmoid.self.methodName()"

    /**
     * Reads the PRIMARY clipboard (X11 mouse selection, whatever was highlighted)
     * and opens the current search engine with it in the browser.
     */
    Q_INVOKABLE void browseWithClipboard();

    /**
     * Opens the current search engine with the given text.
     * Called from the manual search dialog (popup).
     */
    Q_INVOKABLE void browseWithText(const QString &text);

    /**
     * Downloads the favicon of a website and saves it locally.
     * Asynchronous via KIO - does not block the UI!
     * Emits iconFetched() when done.
     */
    Q_INVOKABLE void fetchIcon(const QString &engineName, const QString &pageUrl);

    /**
     * Returns the list of search engines as a QVariantList for QML.
     * Each entry: {name, url, icon, position, hidden}
     */
    Q_INVOKABLE QVariantList engineList() const;

    /** Saves a search engine (create new or update existing). */
    Q_INVOKABLE void saveEngine(const QString &oldName, const QString &newName,
                                const QString &url, const QString &icon,
                                const QString &position, bool hidden);

    /** Deletes a search engine. */
    Q_INVOKABLE void deleteEngine(const QString &name);

    /** Saves all engines to config and rebuilds the context menu. Called from QML config pages. */
    Q_INVOKABLE void rebuildAfterConfigChange();

    /**
     * Cycles to the next (+1) or previous (-1) visible search engine.
     * Called from the mouse wheel handler in QML.
     */
    Q_INVOKABLE void cycleEngine(int direction);

Q_SIGNALS:
    void currentEngineChanged();

    /**
     * Signal: QML should open/close the popup dialog.
     * Triggered by "Manual query..." in the context menu.
     * Received in QML via: Connections { target: Plasmoid; function onRequestTogglePopup() {} }
     */
    void requestTogglePopup();

    /** Feedback after fetchIcon() - provides the file path of the icon, empty on error. */
    void iconFetched(const QString &engineName, const QString &iconPath);

    /** After saveEngine/deleteEngine: the config UI should refresh itself. */
    void enginesChanged();

private Q_SLOTS:
    void setEngineFromAction();   // connected to QAction::triggered() from the context menu
    void onDbusShortcutPressed(const QString &component, const QString &shortcut, qlonglong timestamp);

private:
    void createMenu();
    void openUrl(const QString &engineName, const QString &query);
    void populateEngines();
    void saveEnginesToConfig();

    QString m_currentEngine;
    QActionGroup *m_langChoices = nullptr;
    QList<QAction *> m_actions;
    QHash<QString, Babelengine *> m_enginesHash;
    QStringList m_enginesList;
    KConfigGroup m_configuration;
    QAction *m_manualQueryAction = nullptr;  // global shortcut: open manual query popup
};
