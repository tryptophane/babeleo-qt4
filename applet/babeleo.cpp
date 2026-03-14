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

#include "babeleo.h"

#include <algorithm>

#include <QApplication>
#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#include <KGlobalAccel>
#include <KLocalizedString>
#include <KIO/FavIconRequestJob>
#include <KIO/OpenUrlJob>
#include <KJob>
#include <KMessageBox>
#include <KPluginFactory>
#include <KWaylandExtras>
#include <KWindowSystem>
#include <QFutureWatcher>

// Helper function: loads an icon either from the system theme (if it is a name)
// or from a local file (if it is a path starting with '/').
// Necessary because fetchIcon() saves favicons as files, while the
// predefined icons are theme names like "babelfishleo-en".
static QIcon iconFromValue(const QString &iconValue)
{
    if (iconValue.startsWith(QLatin1Char('/'))) {
        return QIcon(iconValue);
    }
    return QIcon::fromTheme(iconValue);
}

Babeleo::Babeleo(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    KLocalizedString::setApplicationDomain("plasma_applet_babeleo");
}

Babeleo::~Babeleo()
{
    qDeleteAll(m_enginesHash);
}

void Babeleo::init()
{
    m_configuration = config();
    m_currentEngine = m_configuration.readEntry("currentEngine", QStringLiteral("Leo En - De"));
    createMenu();
    if (m_enginesHash.contains(m_currentEngine)) {
        setIcon(m_enginesHash.value(m_currentEngine)->getIcon());
    }

    // Register a global shortcut for opening the manual query popup.
    // Note: translating the clipboard is handled by the built-in "Activate widget as if clicked"
    // shortcut (Plasma::Applet::setGlobalShortcut), configurable in the widget's Shortcuts tab.
    m_manualQueryAction = new QAction(i18n("Open Babeleo manual query"), this);
    m_manualQueryAction->setObjectName(QStringLiteral("manual-query"));
    KGlobalAccel::setGlobalShortcut(m_manualQueryAction, QKeySequence()); // no default shortcut

    // kglobalacceld activates the shortcut via two paths simultaneously:
    // - invokeAction on plasmashell's KGlobalAccel D-Bus → triggers QAction::triggered()
    // - globalShortcutPressed signal on the component object
    // Using only the D-Bus signal path to avoid a double-emit (which would toggle the popup
    // open and immediately closed again, making it appear as if nothing happened).
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.kde.kglobalaccel"),
        QStringLiteral("/component/plasmashell"),
        QStringLiteral("org.kde.kglobalaccel.Component"),
        QStringLiteral("globalShortcutPressed"),
        this,
        SLOT(onDbusShortcutPressed(QString, QString, qlonglong))
    );
}

void Babeleo::configChanged()
{
    // Called by Plasma when config changes externally (e.g. from a QML config page).
    // Reload all settings from KConfig and rebuild the context menu.
    m_currentEngine = m_configuration.readEntry("currentEngine", QStringLiteral("Leo En - De"));
    createMenu();
    if (m_enginesHash.contains(m_currentEngine)) {
        setIcon(m_enginesHash.value(m_currentEngine)->getIcon());
    }
    Q_EMIT currentEngineChanged();
}

void Babeleo::rebuildAfterConfigChange()
{
    saveEnginesToConfig();
    createMenu();
    if (m_enginesHash.contains(m_currentEngine)) {
        setIcon(m_enginesHash.value(m_currentEngine)->getIcon());
    }
    Q_EMIT currentEngineChanged();
    Q_EMIT enginesChanged();
}

void Babeleo::createMenu()
{
    m_enginesList = m_configuration.readEntry("engines", QStringList());
    if (m_enginesList.size() < 5) {
        populateEngines();
    }

    qDeleteAll(m_actions);
    m_actions.clear();
    delete m_langChoices;
    m_langChoices = new QActionGroup(this);
    m_langChoices->setExclusive(true);
    qDeleteAll(m_enginesHash);
    m_enginesHash.clear();

    QStringList sortedNames;
    for (int i = 0; i + 4 < m_enginesList.size(); i += 5) {
        const bool hidden = (m_enginesList.value(i + 4) == QLatin1String("1"));
        Babelengine *engine = new Babelengine(
            m_enginesList.value(i),
            m_enginesList.value(i + 1),
            m_enginesList.value(i + 2),
            m_enginesList.value(i + 3),
            hidden
        );
        m_enginesHash.insert(m_enginesList.value(i), engine);
        sortedNames.append(m_enginesList.value(i));
    }

    std::sort(sortedNames.begin(), sortedNames.end());

    QMenu *otherMenu = new QMenu(i18n("More search engines"));

    for (const QString &name : std::as_const(sortedNames)) {
        Babelengine *eng = m_enginesHash.value(name);
        if (eng->isHidden()) {
            continue;
        }

        QAction *engineAction = new QAction(eng->getName(), this);
        engineAction->setData(eng->getName());
        engineAction->setCheckable(true);
        engineAction->setIcon(iconFromValue(eng->getIcon()));

        if (eng->getPosition() == QLatin1String("0")) {
            m_actions.append(engineAction);
        } else {
            otherMenu->addAction(engineAction);
        }

        m_langChoices->addAction(engineAction);
        connect(engineAction, &QAction::triggered, this, &Babeleo::setEngineFromAction);

        if (m_currentEngine == eng->getName()) {
            engineAction->setChecked(true);
        }
    }

    if (!otherMenu->isEmpty()) {
        QAction *otherAction = new QAction(i18n("More search engines"), this);
        otherAction->setMenu(otherMenu);
        m_actions.append(otherAction);
    } else {
        delete otherMenu;
    }

    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    m_actions.append(separator);

    QAction *manualQueryAction = new QAction(i18n("Manual query..."), this);
    connect(manualQueryAction, &QAction::triggered, this, [this]() {
        Q_EMIT requestTogglePopup();
    });
    m_actions.append(manualQueryAction);

    // All configuration is now available via the standard Plasma settings dialog
    // (right-click → Configure Babeleo Translator), which has Search Engines,
    // Keyboard Shortcuts, and About pages. No separate menu item needed.
}

QList<QAction *> Babeleo::contextualActions()
{
    return m_actions;
}

QString Babeleo::currentEngine() const
{
    return m_currentEngine;
}

void Babeleo::onDbusShortcutPressed(const QString &component, const QString &shortcut, qlonglong timestamp)
{
    Q_UNUSED(timestamp)
    if (component == QLatin1String("plasmashell") && shortcut == QLatin1String("manual-query")) {
        Q_EMIT requestTogglePopup();
    }
}

void Babeleo::setEngineFromAction()
{
    QAction *senderAction = qobject_cast<QAction *>(sender());
    if (!senderAction) {
        return;
    }

    m_currentEngine = senderAction->data().toString();
    senderAction->setChecked(true);

    if (m_enginesHash.contains(m_currentEngine)) {
        setIcon(m_enginesHash.value(m_currentEngine)->getIcon());
    }

    m_configuration.writeEntry("currentEngine", m_currentEngine);
    Q_EMIT configNeedsSaving();
    Q_EMIT currentEngineChanged();
}

void Babeleo::openUrl(const QString &engineName, const QString &query)
{
    if (!m_enginesHash.contains(engineName)) {
        return;
    }

    QString urlTemplate = m_enginesHash.value(engineName)->getURL();
    const QString encodedQuery = QString::fromUtf8(QUrl::toPercentEncoding(query));
    urlTemplate.replace(QLatin1String("%s"), encodedQuery);

    auto *job = new KIO::OpenUrlJob(QUrl(urlTemplate));
    job->setRunExecutables(false); // Security: only open URLs, do not launch programs

    // On Wayland, global shortcuts have no implicit xdg-activation token
    // (unlike mouse clicks which happen on a surface). Without a token,
    // the compositor blocks the browser from coming to the foreground.
    // Solution: explicitly request a token from KWin before starting the job.
    if (KWindowSystem::isPlatformWayland()) {
        // Use any top-level window of plasmashell as the requesting surface.
        const auto windows = QGuiApplication::topLevelWindows();
        QWindow *win = windows.isEmpty() ? nullptr : windows.first();

        auto *watcher = new QFutureWatcher<QString>(job);
        connect(watcher, &QFutureWatcher<QString>::finished, job, [watcher, job]() {
            const QString token = watcher->result();
            if (!token.isEmpty()) {
                KWindowSystem::setCurrentXdgActivationToken(token);
            }
            watcher->deleteLater();
            job->start();
        });
        watcher->setFuture(KWaylandExtras::xdgActivationToken(win, QStringLiteral("org.kde.plasma.babeleo")));
    } else {
        job->start();
    }
}

void Babeleo::browseWithClipboard()
{
    // Priority for the search term:
    // 1. Primary Selection (mouse selection) via Qt directly
    //    - Works reliably on X11
    //    - On Wayland: Qt6 uses zwp_primary_selection_v1 - whether plasmashell
    //      has access to it depends on KWin
    // 2. Klipper via D-Bus
    //    - Klipper has NO endpoint for the Primary Selection!
    //    - getClipboardContents() only returns history entries.
    //    - Useful as a fallback for the regular clipboard (Ctrl+C)
    // 3. Regular clipboard (Ctrl+C) via Qt directly
    //
    // IMPORTANT: Klipper must NOT be queried first, because
    // getClipboardContents() can return a non-empty string
    // (last Ctrl+C entry) even when something else is currently selected.
    // This would prevent us from falling through to the Primary Selection.
    QString query;

    // 1. Attempt: wl-paste --primary (Wayland Primary Selection)
    //    wl-paste (from the optional package wl-clipboard) is a regular
    //    Wayland client that is allowed to read the Primary Selection - plasmashell
    //    itself does not have this permission for security reasons.
    //    Without wl-clipboard: code automatically falls back to Klipper.
    //    findExecutable() prevents an unnecessary 500ms timeout when
    //    wl-paste is not installed.
    if (!QStandardPaths::findExecutable(QStringLiteral("wl-paste")).isEmpty()) {
        QProcess proc;
        proc.start(QStringLiteral("wl-paste"),
                   {QStringLiteral("--primary"), QStringLiteral("--no-newline")});
        if (proc.waitForFinished(500) && proc.exitCode() == 0) {
            query = QString::fromUtf8(proc.readAllStandardOutput());
        }
    }

    // 2. Attempt: QClipboard::Selection (works on X11, rarely on Wayland)
    if (query.isEmpty() && QGuiApplication::clipboard()->supportsSelection()) {
        query = QGuiApplication::clipboard()->text(QClipboard::Selection);
    }

    // 3. Attempt: Klipper via D-Bus (returns regular clipboard / last Ctrl+C)
    if (query.isEmpty()) {
        QDBusInterface klipper(
            QStringLiteral("org.kde.klipper"),
            QStringLiteral("/klipper"),
            QStringLiteral("org.kde.klipper.klipper")
        );
        if (klipper.isValid()) {
            const QDBusReply<QString> reply = klipper.call(QStringLiteral("getClipboardContents"));
            if (reply.isValid()) {
                query = reply.value();
            }
        }
    }

    // 4. Last fallback: Qt directly
    if (query.isEmpty()) {
        query = QGuiApplication::clipboard()->text(QClipboard::Clipboard);
    }

    openUrl(m_currentEngine, query);
}

void Babeleo::browseWithText(const QString &text)
{
    openUrl(m_currentEngine, text);
}

void Babeleo::fetchIcon(const QString &engineName, const QString &pageUrl)
{
    // KIO::FavIconRequestJob is KDE's built-in favicon service.
    // It handles everything: parsing HTML, finding the favicon URL, downloading,
    // converting the format (SVG→PNG), and storing it in a shared cache.
    // (~/.cache/favicons/)
    //
    // We only pass the host URL - scheme and host are sufficient.
    // The job starts automatically (no job->start() needed).
    QString resolvedUrl = pageUrl;
    resolvedUrl.replace(QLatin1String("%s"), QLatin1String("test"));

    auto *job = new KIO::FavIconRequestJob(QUrl(resolvedUrl));
    connect(job, &KJob::result, this, [this, engineName, job]() {
        if (job->error()) {
            qDebug() << "[Babeleo] fetchIcon failed:" << job->errorString();
            Q_EMIT iconFetched(engineName, QString());
            return;
        }
        // Copy the favicon from the volatile KIO cache to a persistent location
        // so it survives cache cleanups.
        // Store favicons under AppDataLocation, NOT under plasma/plasmoids/,
        // because Plasma treats any directory under plasma/plasmoids/<id>/ as a
        // local package override, which shadows the system installation.
        const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                + QStringLiteral("/babeleo/icons/");
        QDir().mkpath(iconDir);
        const QString srcPath  = job->iconFile();
        const QString destPath = iconDir + QFileInfo(srcPath).fileName();
        if (!QFile::exists(destPath))
            QFile::copy(srcPath, destPath);
        qDebug() << "[Babeleo] fetchIcon: Icon stored as" << destPath;
        Q_EMIT iconFetched(engineName, destPath);
    });
}

QVariantList Babeleo::engineList() const
{
    QVariantList result;
    for (auto it = m_enginesHash.constBegin(); it != m_enginesHash.constEnd(); ++it) {
        const Babelengine *eng = it.value();
        QVariantMap entry;
        entry[QStringLiteral("name")]     = eng->getName();
        entry[QStringLiteral("url")]      = eng->getURL();
        entry[QStringLiteral("icon")]     = eng->getIcon();
        entry[QStringLiteral("position")] = eng->getPosition();
        entry[QStringLiteral("hidden")]   = eng->isHidden();
        result.append(entry);
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("name")).toString()
             < b.toMap().value(QStringLiteral("name")).toString();
    });
    return result;
}

void Babeleo::saveEngine(const QString &oldName, const QString &newName,
                         const QString &url, const QString &icon,
                         const QString &position, bool hidden)
{
    if (oldName != newName && m_enginesHash.contains(newName)) {
        KMessageBox::error(nullptr, i18n("A search engine with this name already exists."));
        return;
    }

    if (!oldName.isEmpty()) {
        delete m_enginesHash.take(oldName);
    }

    m_enginesHash.insert(newName, new Babelengine(newName, url, icon, position, hidden));

    saveEnginesToConfig();
    createMenu();
    Q_EMIT enginesChanged();
}

void Babeleo::deleteEngine(const QString &name)
{
    delete m_enginesHash.take(name);
    saveEnginesToConfig();
    createMenu();
    Q_EMIT enginesChanged();
}

void Babeleo::saveEnginesToConfig()
{
    m_enginesList.clear();
    for (auto it = m_enginesHash.constBegin(); it != m_enginesHash.constEnd(); ++it) {
        const Babelengine *eng = it.value();
        m_enginesList << eng->getName()
                      << eng->getURL()
                      << eng->getIcon()
                      << eng->getPosition()
                      << (eng->isHidden() ? QStringLiteral("1") : QStringLiteral("0"));
    }
    m_configuration.writeEntry("engines", m_enginesList);
    Q_EMIT configNeedsSaving();
}

void Babeleo::populateEngines()
{
    m_enginesList.clear();

    m_enginesList << QStringLiteral("Leo En - De")
                  << QStringLiteral("https://dict.leo.org/englisch-deutsch/%s")
                  << QStringLiteral("babelfishleo-en") << QStringLiteral("0") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Leo Fr - De")
                  << QStringLiteral("https://dict.leo.org/franz%C3%B6sisch-deutsch/%s")
                  << QStringLiteral("babelfishleo-fr") << QStringLiteral("0") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Leo Es - De")
                  << QStringLiteral("https://dict.leo.org/spanisch-deutsch/%s")
                  << QStringLiteral("babelfishleo-es") << QStringLiteral("0") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Leo It - De")
                  << QStringLiteral("https://dict.leo.org/italienisch-deutsch/%s")
                  << QStringLiteral("babelfishleo-it") << QStringLiteral("0") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Leo Ch - De")
                  << QStringLiteral("https://dict.leo.org/chinesisch-deutsch/%s")
                  << QStringLiteral("babelfishleo-ch") << QStringLiteral("0") << QStringLiteral("0");

    m_enginesList << QStringLiteral("PONS De - En")
                  << QStringLiteral("https://de.pons.com/%C3%BCbersetzung?q=%s&l=deen")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("PONS deutsch")
                  << QStringLiteral("https://de.pons.com/%C3%BCbersetzung?q=%s&l=dede")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Google")
                  << QStringLiteral("https://www.google.com/search?q=%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Google Translate De - En")
                  << QStringLiteral("https://translate.google.com/?sl=de&tl=en&text=%s&op=translate")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Wikipedia DE")
                  << QStringLiteral("https://de.wikipedia.org/wiki/%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Wikipedia EN")
                  << QStringLiteral("https://en.wikipedia.org/wiki/%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("1");

    m_enginesList << QStringLiteral("Duden")
                  << QStringLiteral("https://www.duden.de/suchen/dudenonline/%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("Google Maps")
                  << QStringLiteral("https://www.google.com/maps/search/?q=%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_enginesList << QStringLiteral("PubMed")
                  << QStringLiteral("https://pubmed.ncbi.nlm.nih.gov/?term=%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("1");

    m_enginesList << QStringLiteral("Real Academia Espanola")
                  << QStringLiteral("https://dle.rae.es/%s")
                  << QStringLiteral("babelfishleo") << QStringLiteral("1") << QStringLiteral("0");

    m_configuration.writeEntry("engines", m_enginesList);
    Q_EMIT configNeedsSaving();
}

K_PLUGIN_CLASS_WITH_JSON(Babeleo, "metadata.json")

#include "babeleo.moc"
