/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.discover

QtObject {
    id: root

    signal currentIndexChanged(currentIndex: int)

    readonly property bool fullScreenModeAvailable: !Kirigami.Settings.isMobile

    enum Mode {
        FullScreen,
        Overlay
    }

    property /*Mode*/ int __mode: CarouselMaximizedViewController.Mode.Overlay

    property CarouselAbstractMaximizedView __view

    function open(transientParent: Window, model: ScreenshotsModel, currentIndex: int) {
        if (__view) {
            if (__view.transientParent === transientParent && __view.model === ScreenshotsModel) {
                __view.currentIndex = currentIndex;
                return;
            } else {
                close(false);
            }
        }
        __view = __createView(transientParent, model, currentIndex);
    }

    function close(animated: bool) {
        __view?.close(animated);
        __view = null;
    }

    function __otherMode() {
        return __mode !== CarouselMaximizedViewController.Mode.FullScreen && fullScreenModeAvailable
            ? CarouselMaximizedViewController.Mode.FullScreen
            : CarouselMaximizedViewController.Mode.Overlay;
    }

    function toggleMode() {
        if (!__view) {
            __mode = __otherMode();
            return;
        }
        const { transientParent, model, currentIndex } = __view;
        close(false);
        __mode = __otherMode();
        open(transientParent, model, currentIndex);
    }

    function __createView(transientParent: Window, model: ScreenshotsModel, currentIndex: int): CarouselAbstractMaximizedView {
        let component = null;
        switch (__mode) {
        case CarouselMaximizedViewController.Mode.FullScreen:
            component = Qt.createComponent("./CarouselFullScreenMaximizedView.qml");
            break;
        case CarouselMaximizedViewController.Mode.Overlay:
            component = Qt.createComponent("./CarouselOverlayMaximizedView.qml");
            break;
        default:
            return null;
        }
        if (!component) {
            console.error("Can not create maximized view component");
            return null;
        }
        const view = component.createObject(this, {
            transientParent,
            model,
            currentIndex,
            toggleModeAvailable: fullScreenModeAvailable
        });
        if (!view) {
            console.error("Can not create maximized view");
            return null;
        }
        return view;
    }

    property list<QtObject> data: [
        Connections {
            target: root.__view

            function onCurrentIndexChanged() {
                root.currentIndexChanged(root.__view.currentIndex)
            }

            function onToggleMode() {
                root.toggleMode();
            }
        }
    ]
}
