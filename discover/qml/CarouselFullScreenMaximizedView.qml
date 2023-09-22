/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.discover

CarouselAbstractMaximizedView {
    id: root

    mode: CarouselMaximizedViewController.Mode.FullScreen

    function close(animated: bool) {
        window.visible = false;
        destroy();
    }

    readonly property Window window: Window {
        id: window

        visible: true
        visibility: Window.FullScreen
        flags: Qt.FramelessWindowHint
        transientParent: root.transientParent

        color: root.backgroundColor

        LayoutMirroring.enabled: transientParent.LayoutMirroring.enabled
        LayoutMirroring.childrenInherit: true

        CarouselMaximizedViewContent {
            anchors.fill: parent
            host: root
        }

        Connections {
            target: window.contentItem.Keys
            function onEscapePressed(event) {
                root.close();
            }
        }

        onVisibleChanged: {
            if (!visible) {
                root.close();
            }
        }
    }
}
