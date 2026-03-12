/***********************************************************************************
* Babeleo: Plasmoid for translating clipboard content at leo.org.
* Copyright (C) 2009 Pascal Pollet <pascal@bongosoft.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
***********************************************************************************/

#include "babelengine.h"

Babelengine::Babelengine(const QString &name, const QString &url, const QString &icon,
                         const QString &position, bool hidden)
    : m_name(name)
    , m_url(url)
    , m_icon(icon)
    , m_position(position)
    , m_hidden(hidden)
{
    // Initialization via member initializer list (more efficient than assignment in the body)
}

void Babelengine::setName(const QString &name)     { m_name = name; }
void Babelengine::setURL(const QString &url)       { m_url = url; }
void Babelengine::setIcon(const QString &icon)     { m_icon = icon; }
void Babelengine::setPosition(const QString &pos)  { m_position = pos; }
void Babelengine::setHidden(bool hidden)           { m_hidden = hidden; }

QString Babelengine::getName() const     { return m_name; }
QString Babelengine::getURL() const      { return m_url; }
QString Babelengine::getIcon() const     { return m_icon; }
QString Babelengine::getPosition() const { return m_position; }
bool    Babelengine::isHidden() const    { return m_hidden; }
