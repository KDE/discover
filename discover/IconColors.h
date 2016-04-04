/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ICONCOLORS_H
#define ICONCOLORS_H

#include <QColor>
#include <QIcon>
#include <QObject>

class IconColors : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName)
    Q_PROPERTY(QColor dominantColor READ dominantColor NOTIFY dominantColorChanged STORED false)
public:
    IconColors(QObject* parent = Q_NULLPTR);

    QString iconName() const;
    void setIconName(const QString& name);

    void setIcon(const QIcon &icon);
    QIcon icon() const { return m_icon; }

    QColor dominantColor() const;

Q_SIGNALS:
    void dominantColorChanged(const QColor &dominantColor);

private:
    QIcon m_icon;
};

#endif // ICONCOLORS_H
