/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.14 as Kirigami

/*
 * An AbstractCard with a default background but without any fancy sizing
 * features of an AbstractCard.
 */
Kirigami.AbstractCard {
    id: root

    property Item content

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            (content?.implicitWidth ?? 0) + leftPadding + rightPadding)

    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             (content?.implicitHeight ?? 0) + topPadding + bottomPadding)

    padding: Kirigami.Units.largeSpacing
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    Component.onCompleted: {
        initContent();
    }

    onContentChanged: {
        initContent();
    }

    function initContent() {
        if (!content) {
            return;
        }

        content.parent = this;
        content.anchors.top = top;
        content.anchors.left = left;
        content.anchors.right = right;
        content.anchors.bottom = bottom;
        content.anchors.topMargin = topPadding;
        content.anchors.leftMargin = Qt.binding(() => mirrored ? rightPadding : leftPadding);
        content.anchors.rightMargin = Qt.binding(() => mirrored ? leftPadding : rightPadding);
        content.anchors.bottomMargin = bottomPadding;
    }
}
