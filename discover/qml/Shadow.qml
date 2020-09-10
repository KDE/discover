/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtGraphicalEffects 1.0
import org.kde.kirigami 2.2

LinearGradient {
    id: shadow
    property int edge: Qt.LeftEdge

    width: Units.gridUnit/2
    height: Units.gridUnit/2

    start: Qt.point((edge !== Qt.RightEdge ? 0 : width), (edge !== Qt.BottomEdge ? 0 : height))
    end: Qt.point((edge !== Qt.LeftEdge ? 0 : width), (edge !== Qt.TopEdge ? 0 : height))
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Theme.backgroundColor
        }
        GradientStop {
            position: 0.3
            color: Qt.rgba(0, 0, 0, 0.1)
        }
        GradientStop {
            position: 1.0
            color:  "transparent"
        }
    }
}

