/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.discover

CarouselAbstractMaximizedView {
    id: root

    mode: CarouselMaximizedViewController.Mode.Overlay

    function close(animated: bool) {
        if (animated) {
            popup.close();
        } else {
            destroy();
        }
    }

    // This is a completely custom Popup for full-scene overlay content. Thus
    // we don't need any styling bits like padding, background or a modal
    // overlay.
    readonly property T.Popup popup: T.Popup {
        id: popup

        Kirigami.OverlayZStacking.layer: Kirigami.OverlayZStacking.FullScreen
        z: Kirigami.OverlayZStacking.z

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                contentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 contentHeight + topPadding + bottomPadding)

        visible: true // show as soon as it is created, destroy immediately after it is closed.
        focus: true
        modal: true
        dim: false // dim component is pointless for a popup that takes up the whole window.
        clip: false // there is no point in clipping overlay that fills whole window at all times.
        closePolicy: T.Popup.CloseOnEscape
        parent: root.transientParent?.T.Overlay.overlay ?? null
        x: 0
        y: 0
        width: parent?.width ?? 0
        height: parent?.height ?? 0

        // Default margins of -1 mean that popup positioner won't even try to
        // push the popup within the bounds of the enclosing window. But we
        // don't care, because we expect it to always fill up exactly the
        // whole window.
        margins: -1
        // All buttons and controls should touch the edge of the enclosing
        // window. They are expected to bring their own padding.
        padding: 0

        contentItem: CarouselMaximizedViewContent {
            host: root
        }

        background: Rectangle {
            color: root.backgroundColor
        }

        T.Overlay.modal: Item { }

        onClosed: {
            root.destroy();
        }
    }
}
