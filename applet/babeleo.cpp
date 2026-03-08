/***********************************************************************************
* Babeleo: Plasmoid for translating clipboard content at leo.org.
* Copyright (C) 2009 Pascal Pollet <pascal@bongosoft.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
***********************************************************************************/

#include "babeleo.h"

#include <kio/netaccess.h>

#include <iostream>

#include <QGraphicsLinearLayout>
#include <QClipboard>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QSize>
#include <QHash>
#include <QMessageBox>
#include <QButtonGroup>
#include <QBrush>
#include <QColor>
#include <QList>
#include <QFile>

#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

#include <KIcon>
#include <KLocale>
#include <KToolInvocation>
#include <KConfigDialog>
#include <KStandardDirs>
#include <KGlobal>
#include <KMessageBox>
//#include <KAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KApplication>
#include <KGlobalAccel>

Babeleo::Babeleo(QObject *parent, const QVariantList &args) : Plasma::Applet(parent, args)
{
    setAspectRatioMode(Plasma::ConstrainedSquare);
    setAcceptDrops(true);

    int iconSize = IconSize(KIconLoader::Desktop);

    resize((iconSize * 2), (iconSize * 2));
}


Babeleo::~Babeleo()
{
}


void Babeleo::init()
{

    //fetchIcon("sciencedirect", "http://www.sciencedirect.com");
    m_icon = new Plasma::IconWidget(KIcon("babelfishleo-en"), QString(), this);
    m_queryButton = new QToolButton;

    m_configuration = config();
    createMenu();

    setFocusPolicy(Qt::StrongFocus);

    registerAsDragHandle(m_icon);

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addItem(m_icon);

    Plasma::ToolTipContent toolTipData(i18n("Translate clipboard content"),
            i18n("current dictionary: ") + m_language ,
            m_icon->icon().pixmap(IconSize(KIconLoader::Desktop)));
    Plasma::ToolTipManager::self()->setContent(this, toolTipData);

    m_dialog = new Plasma::Dialog();
    m_dialog->setWindowFlags(Qt::Popup);
    m_dialog->setResizeHandleCorners(Plasma::Dialog::All);
    m_dialog->setMinimumSize(QSize(200, 80));
    m_dialog->resize(m_configuration.readEntry("dialogSize", m_dialog->sizeHint()));
    
    m_lineEdit = new QLineEdit(m_dialog);
    m_lineEdit->setFocusPolicy(Qt::StrongFocus);
    QVBoxLayout *verticalLayout = new QVBoxLayout(m_dialog);
    verticalLayout->setSpacing(0);
    verticalLayout->setMargin(0);
    verticalLayout->addWidget(m_lineEdit);

    m_queryButton->setIcon(KIcon("babelfishleo-en"));
    m_queryButton->setToolTip(i18n("start query in webbrowser..."));
    
    m_closeButton = new QToolButton;
    m_closeButton->setIcon(KIcon("application-exit"));
    m_closeButton->setToolTip(i18n("close this dialog."));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->setAlignment(Qt::AlignLeft);
    horizontalLayout->addWidget(m_queryButton);
    horizontalLayout->addWidget(m_closeButton);

    verticalLayout->addLayout(horizontalLayout);

    m_lineEdit->setFocusPolicy(Qt::StrongFocus);
    m_dialog->setFocusPolicy(Qt::StrongFocus);

    //for global shortcut:
    KActionCollection *actionCollection = new KActionCollection(this);
    m_newAct = actionCollection->addAction("Babeleo: toggle dialog");
    m_newAct->setObjectName("Babeleo: toggle dialog");
    m_newAct->setGlobalShortcut(m_newAct->globalShortcut(), KAction::ActiveShortcut, KAction::Autoloading);
    //std::cout << "SHORTCUT: " << qPrintable(m_newAct->globalShortcut().toString()) << std::endl;
    
    connect(m_newAct, SIGNAL(triggered()), this, SLOT(toggleDialog()));

    connect(m_icon, SIGNAL(clicked()), this, SLOT(browseEngine()));
    
    connect(m_dialog, SIGNAL(dialogResized()), this, SLOT(dialogResized()));
    connect(m_queryButton, SIGNAL(clicked()), this, SLOT(startManualQuery()));
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(startManualQuery()));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(closeDialog()));
    connect(this, SIGNAL(activate()), this, SLOT(browseEngine())); //for global shortcut

     
}


void Babeleo::createMenu()
{
    
    m_language = m_configuration.readEntry("languages", "Leo En - De");
    m_engines = m_configuration.readEntry("engines", QStringList());
    
    if ( m_engines.size() < 5 ) {
        populateEngines();
    }

    m_langChoices = new QActionGroup(this);
    m_langChoices->setExclusive(true);
    m_actions = new QList<QAction*>;
    m_manualQuery = new QAction(i18n("manual query..."), this);

    connect(m_manualQuery, SIGNAL(triggered(bool)), this, SLOT(toggleDialog()));


    QMenu *m_Menu = new QMenu(i18n("other search engines")); 

    QList<QString> qlist = QList<QString>();
    m_enginesHash = new QHash<QString, Babelengine *>();

    for ( int i = 0; i < m_engines.size(); i += 5 ) {
        bool hidden = true;
        if ( m_engines.value(i+4) == "0" ) {
            hidden = false;
        }
        Babelengine *engine = new Babelengine(m_engines.value(i), m_engines.value(i+1), m_engines.value(i+2), m_engines.value(i+3), hidden);
        m_enginesHash->insert(m_engines.value(i), engine);

       qlist.append(m_engines.value(i));
    }
    
    qSort(qlist);

    for (int i = 0; i < qlist.size(); ++i) {
        Babelengine *eng = m_enginesHash->value(qlist.at(i));

        QAction *engineAction = new QAction(eng->getName(), this);
        engineAction->setData(eng->getName());
        engineAction->setCheckable(true);
        engineAction->setIcon(KIcon(eng->getIcon()));

        if ( ! eng->isHidden() ) {
            if ( eng->getPosition() == "0" ) {
                m_actions->append( engineAction );
            }
            else {
                m_Menu->addAction( engineAction );
            }
        }
        
        m_langChoices->addAction(engineAction);

        connect(engineAction, SIGNAL(triggered(bool)), this, SLOT(setEngine()));

        if ( m_language == eng->getName() ) {
            engineAction->setChecked(true);
            m_icon->setIcon(engineAction->icon());
            m_queryButton->setIcon(engineAction->icon());
        }
    }

    QAction *other = new QAction(i18n("other search engines"), this);
    other->setMenu(m_Menu);
    m_actions->append(other);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    m_actions->append(separator);
    m_actions->append(m_manualQuery);
}


QList<QAction*> Babeleo::contextualActions()
{
   return *m_actions;
}

void Babeleo::populateEngines()
{
    m_engines << "Leo En - De" << QString("http://dict.leo.org/ende?lp=ende&lang=de&searchLoc=0&cmpType=relaxed") +
            QString("&sectHdr=on&spellToler=on&chinese=both&pinyin=diacritic&search=%s&relink=on") << QString("babelfishleo-en")\
            << QString("0") << QString("0");
    m_engines << "Leo Fr - De" << QString("http://dict.leo.org/frde?lp=frde&lang=de&searchLoc=0&cmpType=relaxed") +
            QString("&sectHdr=on&spellToler=on&chinese=both&pinyin=diacritic&search=%s&relink=on") << QString("babelfishleo-fr")\
                        << QString("0") << QString("0");
    m_engines << "Leo Es - De" << QString("http://dict.leo.org/esde?lp=esde&lang=de&searchLoc=0&cmpType=relaxed") +
            QString("&sectHdr=on&spellToler=on&chinese=both&pinyin=diacritic&search=%s&relink=on") << QString("babelfishleo-es")\
                        << QString("0") << QString("0");
    m_engines << "Leo It - De" << QString("http://dict.leo.org/itde?lp=itde&lang=de&searchLoc=0&cmpType=relaxed") +
            QString("&sectHdr=on&spellToler=on&chinese=both&pinyin=diacritic&search=%s&relink=on") << QString("babelfishleo-it")\
                                    << QString("0") << QString("0");
    m_engines << "Leo Ch - De" << QString("http://dict.leo.org/chde?lp=chde&lang=de&searchLoc=0&cmpType=relaxed") +
            QString("&sectHdr=on&spellToler=on&chinese=both&pinyin=diacritic&search=%s&relink=on") << QString("babelfishleo-ch")\
                                    << QString("0") << QString("0");

    // add new search engines here. %s stands for the query word.
    m_engines << "Canoo.net" << "http://www.canoo.net/services/Controller?input=%s&MenuId=Search&service=canooNet&lang=de" << QString("babelfishleo")\
                                << QString("1") << QString("0");
    m_engines << "Duden" << "http://www.duden-suche.de/suche/trefferliste.php?suchbegriff[AND]=%s" << QString("babelfishleo")\
                                        << QString("1") << QString("1");
    m_engines << "Google" << "http://www.google.de/search?hl=de&source=hp&q=%s&btnG=Google-Suche" << QString("babelfishleo")\
                                        << QString("1") << QString("0");
    m_engines << "Iate" << QString("http://iate.europa.eu/iatediff/SearchByQuery.do?method=search") + 
        QString("&saveStats=true&screenSize=1680x1050&query=%s&valid=Search+&sourceLanguage=de&targetLanguages=s&domain=0&typeOfSearch=s&request=")\
        << QString("babelfishleo") << QString("1") << QString("1");
    m_engines << "Open Dictionary" << "http://open-dictionary.com/%s" << QString("babelfishleo") << QString("1") << QString("0");
    m_engines << "PONS deutsch" << "http://www.pons.eu/dict/search/results/?q=%s&in=&l=dede" << QString("babelfishleo")\
                                        << QString("1") << QString("0");
    m_engines << "PONS De - En" << "http://www.pons.eu/dict/search/results/?q=%s&in=&l=deen" << QString("babelfishleo")\
                                        << QString("1") << QString("1");
    m_engines << "Wikipedia" << "http://de.wikipedia.org/wiki/%s" << QString("babelfishleo")\
                                        << QString("1") << QString("0");
    m_engines << "Wortschatz Uni Leipzig" << "http://wortschatz.informatik.uni-leipzig.de/cgi-bin/wort_www.exe?site=1&Wort=%s&sprache=de"\
                                        << QString("babelfishleo")  << QString("1") << QString("0");
    m_engines << "CNRS synonymes" << "http://www.crisco.unicaen.fr/cgi-bin/trouvebis2?requete=%s" << QString("babelfishleo")\
                                        << QString("1") << QString("1");
    m_engines << "Real Academia Espanola" << "http://buscon.rae.es/draeI/SrvltConsulta?TIPO_BUS=3&LEMA=%s" << QString("babelfishleo")\
                                        << QString("1") << QString("0");
    m_engines << "Webster's online dictionary" << "http://www.websters-online-dictionary.org/definition/%s" << QString("babelfishleo")\
                                        << QString("1") << QString("0");
    m_engines << "GDT FR-EN" << "http://www.granddictionnaire.com/btml/fra/r_motclef/Recherche_FR.asp?iPointAcces=71zaq3121&sMotClefTraite=%s\",\"rels\":[],\"params\":[]}],\"_installLocation\":2,\"type\":3,\"queryCharset\":\"UTF-8\",\"_readOnly\":false"\
                                         << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "GDT EN-FR" << "http://www.granddictionnaire.com/btml/fra/r_motclef/Recherche_EN.asp?iPointAcces=72pli645&sMotClefTraite=%s\",\"rels\":[],\"params\":[]}],\"_installLocation\":2,\"type\":3,\"queryCharset\":\"UTF-8\",\"_readOnly\":false"\
                                         << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Diccionario de la lengua espanola" << "http://buscon.rae.es/draeI/SrvltConsulta?TIPO_BUS=3&LEMA=%s"\
                                         << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Diccionario panhispanico de dudas" << "http://buscon.rae.es/dpdI/SrvltConsulta?lema=%s"\
                                         << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Dix" << "http://dix.osola.com/index.de.php?search=%s" << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Le Dictionnaire" << "http://www.le-dictionnaire.com/definition.php?mot=%s" << QString("babelfishleo")  << QString("1") << QString("0");
    m_engines << "Google De - En" << "http://translate.google.de/translate_t?prev=hp&hl=en&js=y&text=%s&file=&sl=de&tl=en&history_state0=#de|en|%s"\
                                         << QString("babelfishleo")  << QString("1") << QString("0");
    m_engines << "Google es - En" << "http://translate.google.de/translate_t?prev=hp&hl=en&js=y&text=%s&file=&sl=es&tl=en&history_state0=#es|en|%s"\
                                         << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Google It - En" << "http://translate.google.de/translate_t?prev=hp&hl=en&js=y&text=%s&file=&sl=it&tl=en&history_state0=#it|en|%s"\
                                                 << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Google Fr - En" << "http://translate.google.de/translate_t?prev=hp&hl=en&js=y&text=%s&file=&sl=fr&tl=en&history_state0=#fr|en|%s"\
                                                 << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Google Pl - En" << "http://translate.google.de/translate_t?prev=hp&hl=en&js=y&text=%s&file=&sl=pl&tl=en&history_state0=#pl|en|%s"\
                                                 << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Pirate Bay" << "http://thepiratebay.org/search/%s/0/99/0" << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "Mininova" << "http://www.mininova.org/search/?search=%s&cat=0" << QString("babelfishleo")  << QString("1") << QString("1");
    m_engines << "PubMed" << "http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?CMD=search&DB=pubmed&term=%s"\
                                                 << QString("babelfishleo")  << QString("1") << QString("1");

    m_configuration.writeEntry("engines", m_engines);
    emit configNeedsSaving();
}

void Babeleo::setEngine() {

    QAction *senderAction = qobject_cast<QAction *>(sender());
   
    m_language = senderAction->data().toString(); 
    senderAction->setChecked(true);
    
    m_icon->setIcon(senderAction->icon());
    m_queryButton->setIcon(senderAction->icon());

    Plasma::ToolTipContent toolTipData(i18n("translate clipboard content at Leo.org"),
            i18n("current dictionary: ") + m_language ,
            m_icon->icon().pixmap(IconSize(KIconLoader::Desktop)));
    Plasma::ToolTipManager::self()->setContent(this, toolTipData);


    m_configuration.writeEntry("languages", m_language);
    emit configNeedsSaving();
}


void Babeleo::browseEngine()
{
    QString query = QApplication::clipboard()->text(QClipboard::Selection);

    QAction *act = m_langChoices->checkedAction();
    QString text = act->data().toString();

    QString URL = m_enginesHash->value(text)->getURL();
    URL.replace(QString("%s"), query);
    KToolInvocation::invokeBrowser(URL);
}

void Babeleo::startManualQuery()
{
    QString query = m_lineEdit->text();
    QAction *act = m_langChoices->checkedAction();
    QString text = act->data().toString();
    QString URL = m_enginesHash->value(text)->getURL();
    URL.replace(QString("%s"), query);
    KToolInvocation::invokeBrowser(URL);

    closeDialog();
}

void Babeleo::toggleDialog()
{
    std::cout << "HHOER" << std::endl;
    if (m_dialog->isVisible())
    {
        m_dialog->hide();
        m_lineEdit->clear();
    }
    else
    {
        m_lineEdit->clear();
        m_dialog->move(popupPosition(m_dialog->sizeHint()));
        m_dialog->show();
        m_lineEdit->setFocus();
    }
}

void Babeleo::fetchIcon()
{
    QListWidgetItem *current = m_uiSettings.listWidget->currentItem();

    QString engineName = current->text();
    QString engineURL = m_uiSettings.URLField->text();

    KStandardDirs *dirs = KGlobal::dirs();
    engineURL.replace(QString("%s"), QString("hello"));

    // adapted from http://www.peej.co.uk/projects/favatars.html Thanx!
    QRegExp *regexp = new QRegExp("<link[^>]+rel=\"(?:shortcut )icon\"[^>]*href=\"([^\"]+)\"");

    QUrl qurl = QUrl(engineURL);
    QString faviconURL;
    QString tmpFile;

    if( KIO::NetAccess::download( KUrl(engineURL), tmpFile, m_uiSettings.iconRequester) )
    {
        QFile tmp(tmpFile);
        if (!tmp.open(QIODevice::ReadOnly | QIODevice::Text)) {
            KMessageBox::error(m_uiSettings.iconRequester, QString("error while downloading icon.") );
            return;
        }
        QByteArray tmpAll = tmp.readAll();
        KIO::NetAccess::removeTempFile( tmpFile );

        if (tmpAll.isEmpty()) {
            KMessageBox::error(m_uiSettings.iconRequester, QString("error while downloading icon.") );
            return;
        }

        if (regexp->indexIn(QString(tmpAll)) != -1) {
            QString iconURL = regexp->cap(1);

            if ( iconURL.left(1) == "/")  {
                faviconURL = qurl.scheme() + "://" + qurl.host() + iconURL;
            }
            else if (iconURL.left(7) == "http://") {
                faviconURL = iconURL;
            }
            else if ( engineURL.right(1) == "/") {
                faviconURL = engineURL + iconURL;
            }
            else faviconURL = engineURL + "/" + iconURL;
        }
        else {
            faviconURL = qurl.scheme() + "://" + qurl.host() + "/favicon.ico";
        }
            
        QString iconDir = dirs->saveLocation("icon", "babeleo", true);
        QString iconFile = iconDir + engineName + ".ico";

        if ( KIO::NetAccess::download( KUrl(faviconURL), iconFile, new QWidget()) ) {
            current->setIcon(KIcon(iconFile));
            m_uiSettings.iconRequester->setUrl(KUrl(iconFile));
        }
        else {
            KMessageBox::error(m_uiSettings.iconRequester, QString("error while downloading icon.") );
            return;
        }
    }

    else
    {
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        KMessageBox::error(m_uiSettings.iconRequester, QString("error while downloading icon.") );
    }


}


void Babeleo::closeDialog()
{
    if (m_dialog->isVisible())
    {
        m_lineEdit->releaseKeyboard();
        m_dialog->hide();
        m_lineEdit->clear();
    }
}


void Babeleo::dialogResized()
{
    m_configuration.writeEntry("dialogSize", m_dialog->size());

    emit configNeedsSaving();
}

void Babeleo::createConfigurationInterface(KConfigDialog *parent)
{
    m_count = 0;
    QDialog *dialogSettings = new QDialog;
    m_uiSettings.setupUi(dialogSettings);
    parent->addPage(dialogSettings, i18nc("search engines", "Search Engines"), "babelfishleo");
    QHashIterator<QString, Babelengine *> i(*m_enginesHash);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item = new QListWidgetItem(i.key(), m_uiSettings.listWidget);
        Babelengine *eng = i.value();
        QString icon = eng->getIcon();
        item->setIcon(KIcon(icon));
        if (eng->isHidden()) {
            item->setForeground(QBrush(QColor("gray"), Qt::SolidPattern));
        }
        m_uiSettings.listWidget->addItem(item);
    }
    
    m_uiSettings.listWidget->sortItems(); 
    m_uiSettings.listWidget->setCurrentRow(0);

    QButtonGroup *buttonGroup = new QButtonGroup(parent);
    buttonGroup->addButton(m_uiSettings.mainMenuRadio);
    buttonGroup->addButton(m_uiSettings.otherRadio);
    buttonGroup->setExclusive(true);
   

    connect(m_uiSettings.listWidget, SIGNAL(currentItemChanged ( QListWidgetItem *, QListWidgetItem * )), this, SLOT(settingsItemChanged( QListWidgetItem *, QListWidgetItem *)));
    connect(m_uiSettings.addButton, SIGNAL(clicked()), this, SLOT(addEngine()));
    connect(m_uiSettings.deleteButton, SIGNAL(clicked()), this, SLOT(deleteEngine()));
    connect(m_uiSettings.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(m_uiSettings.buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(rejectClicked()));
    connect(m_uiSettings.fetchButton, SIGNAL(clicked()), this, SLOT(fetchIcon()));


    connect(parent, SIGNAL(okClicked()), this, SLOT(okClicked()));
    settingsItemChanged(m_uiSettings.listWidget->item(0), m_uiSettings.listWidget->item(0));

    QDialog *dialogOther = new QDialog;
    m_uiOtherSettings.setupUi(dialogOther);
    parent->addPage(dialogOther, i18nc("other options", "Other options"), "preferences-system");

    m_uiOtherSettings.manualQueryShortcutWidget->setKeySequence(m_newAct->globalShortcut().primary());


}

void Babeleo::okClicked() {
    
    QString shortcutManualOld = m_newAct->globalShortcut().primary().toString();
    QString shortcutManualNew = m_uiOtherSettings.manualQueryShortcutWidget->keySequence().toString();

    if ( shortcutManualOld != shortcutManualNew ) {
        m_newAct->setGlobalShortcut(KShortcut(m_uiOtherSettings.manualQueryShortcutWidget->keySequence()), KAction::ActiveShortcut, KAction::NoAutoloading);
    }

    checkEngineChanges(m_uiSettings.listWidget->currentItem(), true);
    saveEngines();
}

void Babeleo::applyClicked() 
{
    checkEngineChanges(m_uiSettings.listWidget->currentItem(), false);
}


void Babeleo::rejectClicked()
{
    QString key = m_uiSettings.listWidget->currentItem()->text();
    Babelengine *eng = m_enginesHash->value(key);
    m_uiSettings.nameField->setText(eng->getName());
    m_uiSettings.URLField->setText(eng->getURL());
    m_uiSettings.iconRequester->setUrl(KUrl(eng->getIcon()));
    if ( eng->isHidden() ) {
        m_uiSettings.hideBox->setChecked(true);
    }
    else {
        m_uiSettings.hideBox->setChecked(false);
    }
    m_uiSettings.nameField->setCursorPosition(0);
    m_uiSettings.URLField->setCursorPosition(0);

}

void Babeleo::saveEngines() {
    QHashIterator<QString, Babelengine *> i(*m_enginesHash);
    m_engines.clear();
    while (i.hasNext()) {
        i.next();
        Babelengine *eng = i.value();
        QString hidden = "0";
        if ( eng->isHidden() ) {
            hidden = "1";
        }
       m_engines << eng->getName() << eng->getURL() << eng->getIcon() << eng->getPosition() << hidden;
    }
    m_configuration.writeEntry("engines", m_engines);
    emit configNeedsSaving();
    createMenu();
}

void Babeleo::addEngine()
{
    QListWidgetItem *item = new QListWidgetItem("new engine", m_uiSettings.listWidget);
    item->setIcon(KIcon("babelfishleo"));
    m_enginesHash->insert("new engine", new Babelengine("new engine", "http://", "babelfishleo", "1", false));
    m_uiSettings.listWidget->addItem(item);
    m_uiSettings.listWidget->sortItems();
    m_count = 0;
    m_uiSettings.listWidget->setCurrentItem(item);
}

void Babeleo::deleteEngine()
{
    QListWidgetItem *current = m_uiSettings.listWidget->currentItem();
    m_enginesHash->remove(current->text());
    m_count=0;
    if ( m_uiSettings.listWidget->currentRow() != 0 ) {
        m_uiSettings.listWidget->setCurrentRow(0);
    }
    else {
        m_uiSettings.listWidget->setCurrentRow(1);
    }
    m_uiSettings.listWidget->takeItem(m_uiSettings.listWidget->row(current));
    saveEngines();
}

void Babeleo::settingsItemChanged(QListWidgetItem *actual, QListWidgetItem *previous)
{
    int ret = 0;
    if (m_count == 0) {
        m_count++;
    }
    else {
        ret = checkEngineChanges(previous, true);
    }

    if ( ret != 0 ) {
        m_count = 0;
        m_uiSettings.listWidget->setCurrentItem(previous);
        return;
    }
    
    else {
        m_uiSettings.listWidget->setCurrentItem(actual);
    }


    m_uiSettings.nameField->setText(actual->text());
    m_uiSettings.nameField->setCursorPosition(0);
    m_uiSettings.URLField->setText(m_enginesHash->value(actual->text())->getURL());
    m_uiSettings.URLField->setCursorPosition(0);
    m_uiSettings.iconRequester->setUrl(KUrl(m_enginesHash->value(actual->text())->getIcon()));
    m_uiSettings.hideBox->setChecked(m_enginesHash->value(actual->text())->isHidden());
    if ( m_enginesHash->value(actual->text())->getPosition() == "0" ) {
        m_uiSettings.mainMenuRadio->setChecked(true);
    }
    else {
        m_uiSettings.otherRadio->setChecked(true);
    }
}

int Babeleo::checkEngineChanges(QListWidgetItem *item, bool ask) {

    item->setIcon(KIcon(m_uiSettings.iconRequester->text()));

    Babelengine *eng = m_enginesHash->value(item->text());
    if ( m_uiSettings.nameField->text() != eng->getName() || \
            m_uiSettings.URLField->text() != eng->getURL() || \
            m_uiSettings.iconRequester->text() != eng->getIcon() || \
            (m_uiSettings.hideBox->isChecked() && ! eng->isHidden()) || \
            (! m_uiSettings.hideBox->isChecked() && eng->isHidden()) || \
            (m_uiSettings.otherRadio->isChecked() && eng->getPosition() == "0") || \
            (! m_uiSettings.otherRadio->isChecked() && eng->getPosition() == "1")) 
    {
        if (ask) {
            QMessageBox msgBox;
            msgBox.setText(i18n("The search engine has been modified."));
            msgBox.setInformativeText("Do you want to save your changes?");
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
            msgBox.setDefaultButton(QMessageBox::Save);
            int ret = msgBox.exec();

            switch (ret) {
                case QMessageBox::Save:
                    return saveChanges(item);
                    break;

                case QMessageBox::Discard:
                    break;

                default:
                    break;

            }   
        }
        else {
            return saveChanges(item);
        }
    }
    return 0;
}


int Babeleo::saveChanges(QListWidgetItem *item)
{
     if (m_enginesHash->contains(m_uiSettings.nameField->text()) && m_uiSettings.nameField->text() != item->text()) {
        QMessageBox msgBox;
        msgBox.setText(i18n("There is already a search engine with this name."));
        msgBox.exec();
        return 1;
    }

    QString position = QString("0");
    if (m_uiSettings.otherRadio->isChecked()) {
        position = QString("1");
    }

    Babelengine *newEngine = new Babelengine(m_uiSettings.nameField->text(), m_uiSettings.URLField->text(),\
            m_uiSettings.iconRequester->text(), position, m_uiSettings.hideBox->isChecked());

    QListWidgetItem *it = new QListWidgetItem(m_uiSettings.nameField->text(), m_uiSettings.listWidget);
    it->setIcon(KIcon(m_uiSettings.iconRequester->text()));

    QString hideChecked = QString("0");
    if (m_uiSettings.hideBox->isChecked()) {
        hideChecked = QString("1");
        it->setForeground(QBrush(QColor("gray"), Qt::SolidPattern));

    }

 
    m_enginesHash->remove(item->text());
    m_enginesHash->insert(m_uiSettings.nameField->text(), newEngine);

    m_count=0;
    if ( m_uiSettings.listWidget->currentRow() != 0 ) {
        m_uiSettings.listWidget->setCurrentRow(0);
    }
    else {
        m_uiSettings.listWidget->setCurrentRow(1);
    }

    m_uiSettings.listWidget->removeItemWidget(item);
    delete item;
    m_uiSettings.listWidget->addItem(it);
    m_uiSettings.listWidget->sortItems();
    m_uiSettings.listWidget->setCurrentItem(it);
    saveEngines();

    return 0;
}

#include "babeleo.moc"
