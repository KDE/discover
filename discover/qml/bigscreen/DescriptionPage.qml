// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Templates as T

import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

// The full description of an app, which the arrow keys scroll through
BigscreenUI.ScrollablePage {
    id: root

    required property Discover.AbstractResource application

    title: application.name
    focus: true

    BigscreenUI.BackHandler.onActivated: T.StackView.view.pop()

    Keys.onUpPressed: scrollTo(flickable.contentY - flickable.height / 3)
    Keys.onDownPressed: scrollTo(flickable.contentY + flickable.height / 3)

    BigscreenUI.Label {
        text: root.application.longDescription
        textFormat: Text.StyledText
        lineHeight: 1.2
        wrapMode: Text.Wrap
    }
}
