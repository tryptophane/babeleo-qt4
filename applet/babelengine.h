/***********************************************************************************
* Babeleo: Plasmoid for translating clipboard content at leo.org.
* Copyright (C) 2009 Pascal Pollet <pascal@bongosoft.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
***********************************************************************************/

#pragma once

#include <QString>

/**
 * Data class for a search engine.
 *
 * Stores: name, URL (with %s as placeholder for the search term),
 * icon name, position in the menu, and whether it is hidden.
 *
 * This is a simple "Value Object" class (comparable to a
 * Java Bean / POJO) - no signals, no Qt meta-system required.
 */
class Babelengine
{
public:
    // const QString& instead of QString: passing by reference avoids unnecessary copies
    Babelengine(const QString &name, const QString &url, const QString &icon,
                const QString &position, bool hidden);

    void setName(const QString &name);
    void setURL(const QString &url);
    void setIcon(const QString &icon);
    void setPosition(const QString &position);
    void setHidden(bool hidden);

    QString getName() const;
    QString getURL() const;
    QString getIcon() const;
    QString getPosition() const;
    bool isHidden() const;

private:
    QString m_name;
    QString m_url;
    QString m_icon;
    QString m_position;
    bool m_hidden;
};
