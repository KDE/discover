/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2

MouseArea {
    id: root

    property alias interactive: indicator.interactive
    property alias currentIndex: indicator.currentIndex
    property alias count: indicator.count
    property alias focusPolicy: indicator.focusPolicy

    property alias bottomPadding: indicator.bottomPadding

    implicitHeight: indicator.implicitHeight
    implicitWidth: indicator.implicitWidth

    QQC2.PageIndicator {
        id: indicator

        LayoutMirroring.enabled: root.LayoutMirroring.enabled

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    onPressed: event => {
        let direction = 0;
        if (event.x < indicator.x) {
            direction = LayoutMirroring.enabled ? 1 : -1;
        } else if (event.x > indicator.x + indicator.width) {
            direction = LayoutMirroring.enabled ? -1 : 1;
        } else {
            return;
        }
        if (direction < 0) {
            if (indicator.currentIndex > 0) {
                indicator.currentIndex -= 1;
            }
        } else if (direction > 0) {
            if (indicator.currentIndex < indicator.count - 1) {
                indicator.currentIndex += 1;
            }
        }
    }
}
