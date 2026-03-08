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

#ifndef BABELEO_HEADER
#define BABELEO_HEADER

#include "babelengine.h"
#include "ui_babeleoSettings.h"
#include "ui_babeleoOtherSettings.h"

#include <Plasma/Applet>
#include <Plasma/IconWidget>
#include <Plasma/Dialog>

#include <kiconloader.h>

#include <plasma/containment.h>

#include <QActionGroup>
#include <QAction>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QListWidgetItem>

#include <KAction>

class Babeleo : public Plasma::Applet
{
    Q_OBJECT

    public:
        Babeleo(QObject *parent, const QVariantList &args);
        ~Babeleo();

        void init();
        void createMenu();
        virtual QList<QAction*> contextualActions();
        void populateEngines();
        int checkEngineChanges(QListWidgetItem *item, bool);
        int saveChanges(QListWidgetItem *item);

    public slots:
        void browseEngine();
        void toggleDialog();
        void closeDialog();
        void dialogResized();
        void setEngine();
        void startManualQuery();
        void settingsItemChanged(QListWidgetItem *, QListWidgetItem *);
        void addEngine();
        void deleteEngine(); 
        void saveEngines();
        void okClicked();
        void applyClicked();
        void rejectClicked();
        void fetchIcon();

    private:
        Plasma::IconWidget *m_icon;
        QString m_language;
        QActionGroup *m_langChoices;
        QAction *m_manualQuery;
        QList<QAction*> *m_actions;
        Plasma::Dialog *m_dialog;
        QLineEdit *m_lineEdit;
        QToolButton *m_queryButton;
        QToolButton *m_closeButton;
        QHash<QString,Babelengine *> *m_enginesHash;
        QString m_engine;
        Ui::settingsDialog m_uiSettings;
        Ui::babeleoOtherSettings m_uiOtherSettings;
        QStringList m_engines;
        int m_count;
        KConfigGroup m_configuration;
        KAction *m_newAct;

    protected:
        void createConfigurationInterface(KConfigDialog *parent);

    signals:


};

K_EXPORT_PLASMA_APPLET(babeleo, Babeleo)
#endif
