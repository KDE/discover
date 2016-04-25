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
import QtQuick.Controls 1.1
import org.kde.discover 1.0

Item
{
    id: item
    visible: model.shouldShow
    height: content.height+10

    signal markUseful(bool useful)

    function usefulnessToString(favorable, total)
    {
        return total==0
                ? i18n("<em>Tell us about this review!</em>")
                : i18n("<em>%1 out of %2 people found this review useful</em>", favorable, total)
    }
    MouseArea {
        id: delegateArea
        hoverEnabled: true
        width: parent.width
        height: content.height

        Label {
            anchors {
                left: parent.left
                right: rating.left
            }

            id: content
            text: i18n("<p style='margin: 0 0 0 0'><b>%1</b> by %2</p><p style='margin: 0 0 0 0'>%3</p><p style='margin: 0 0 0 0'>%4</p>", summary, reviewer,
                    display, usefulnessToString(usefulnessFavorable, usefulnessTotal))
            wrapMode: Text.WordWrap
        }

        Label {
            anchors {
                right: parent.right
                bottom: parent.bottom
                bottomMargin: -5
            }
            opacity: delegateArea.containsMouse ? 1 : 0.2

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

        Rating {
            id: rating
            anchors.top: parent.top
            anchors.right: parent.right
            rating: model.rating
            starSize: content.font.pointSize
        }
    }
}
