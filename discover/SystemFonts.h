/*
 *   Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

#ifndef SYSTEMFONTS_H
#define SYSTEMFONTS_H

#include <QObject>
#include <QFont>

class SystemFonts : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QFont generalFont READ generalFont NOTIFY fontsChanged)
    Q_PROPERTY(QFont fixedFont READ fixedFont NOTIFY fontsChanged)
    Q_PROPERTY(QFont titleFont READ titleFont NOTIFY fontsChanged)
    Q_PROPERTY(QFont smallestReadableFont READ smallestReadableFont NOTIFY fontsChanged)
    public:
        SystemFonts(QObject* parent = nullptr);

        QFont generalFont() const;
        QFont fixedFont() const;
        QFont titleFont() const;
        QFont smallestReadableFont() const;

        bool eventFilter(QObject* obj, QEvent* ev) override;

    Q_SIGNALS:
        void fontsChanged();
};

#endif // SYSTEMFONTS_H
