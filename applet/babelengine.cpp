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

#include "babelengine.h"

Babelengine::Babelengine(QString name, QString url, QString icon, QString position, bool hidden) 
{
    m_name = name;
    m_url = url;
    m_icon = icon;
    m_position = position;
    m_hidden = hidden;
}

void Babelengine::setName(QString name)
{
    m_name = name;
}

void Babelengine::setURL(QString url)
{
    m_url = url;
}

void Babelengine::setIcon(QString icon)
{
    m_icon = icon;
}

void Babelengine::setPosition(QString position)
{
    m_position = position;
}

void Babelengine::setHidden(bool hidden)
{
    m_hidden = hidden;
}

QString Babelengine::getName()
{
    return m_name;
}

QString Babelengine::getURL()
{
    return m_url;
}

QString Babelengine::getIcon()
{
    return m_icon;
}

QString Babelengine::getPosition()
{
    return m_position;
}

bool Babelengine::isHidden()
{
    return m_hidden;
}

