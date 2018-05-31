/*
 *   Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.8
import QtQuick.Controls 2.1
import org.kde.kirigami 2.1 as Kirigami

Popup {
    id: overlay
    parent: applicationWindow().overlay
    bottomPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.largeSpacing

    x: (parent.width - width)/2
    y: (parent.height - height)/2
    width: Math.min(parent.width - Kirigami.Units.gridUnit * 3, Kirigami.Units.gridUnit * 50)
    height: Math.min(view.contentHeight + bottomPadding + topPadding, parent.height * 4/5)
}
