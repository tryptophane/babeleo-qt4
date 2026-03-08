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

#ifndef BABELENGINE_HEADER
#define BABELENGINE_HEADER

#include <QString>

class Babelengine {
    private:
        QString m_name;
        QString m_url;
        QString m_icon;
        QString m_position;
        bool m_hidden;

    public:
        Babelengine(QString name, QString url, QString icon, QString position, bool hidden);
        void setName(QString name);
        void setURL(QString url);
        void setIcon(QString icon);
        void setPosition(QString position);
        void setHidden(bool hidden);
        QString getName();
        QString getURL();
        QString getIcon();
        QString getPosition();
        bool isHidden();
};

#endif
