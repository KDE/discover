/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 1.0
import org.kde.discover 1.0

AbstractListItem
{
    id: item
    visible: model.shouldShow
    height: Math.max(layout.implicitHeight, 0) + 2*Units.smallSpacing

    signal markUseful(bool useful)

    function usefulnessToString(favorable, total)
    {
        return total==0
                ? i18n("<em>Tell us about this review!</em>")
                : i18n("<em>%1 out of %2 people found this review useful</em>", favorable, total)
    }
    ColumnLayout {
        id: layout
        anchors {
            left: parent.left
            right: parent.right
            margins: Units.gridUnit
        }
        Layout.fillWidth: true

        RowLayout {
            Layout.fillWidth: true
            Label {
                id: content
                Layout.fillWidth: true
                text: i18n("<b>%1</b> by %2", summary, reviewer)
            }
            Rating {
                id: rating
                rating: model.rating
                starSize: content.font.pointSize
            }
        }
        Label {
            Layout.fillWidth: true
            text: display
            wrapMode: Text.Wrap
        }
        Label {
            text: usefulnessToString(usefulnessFavorable, usefulnessTotal)
        }

        Label {
            opacity: item.containsMouse ? 1 : 0.2

            text: {
                switch(usefulChoice) {
                    case ReviewsModel.Yes:
                        i18n("<em>Useful? <a href='true'><b>Yes</b></a>/<a href='false'>No</a></em>")
                        break;
                    case ReviewsModel.No:
                        i18n("<em>Useful? <a href='true'>Yes</a>/<a href='false'><b>No</b></a></em>")
                        break;
                    default:
                        i18n("<em>Useful? <a href='true'>Yes</a>/<a href='false'>No</a></em>")
                        break;
                }
            }
            onLinkActivated: item.markUseful(link=='true')
        }
    }
}
