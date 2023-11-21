/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick

import org.kde.kirigami as Kirigami

Item {
    id: root

    // See documentation in CarouselNavigationButton.qml
    required property bool animated
    required property bool atBeginning
    required property bool atEnd

    property real edgeMargin: Kirigami.Units.gridUnit

    signal decrementCurrentIndex()
    signal incrementCurrentIndex()

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    anchors.fill: parent

    CarouselNavigationButton {
        Kirigami.Theme.inherit: true
        LayoutMirroring.enabled: root.LayoutMirroring.enabled

        animated: root.animated
        atBeginning: root.atBeginning
        atEnd: root.atEnd
        edgeMargin: root.edgeMargin
        role: Qt.AlignLeading

        onClicked: {
            root.decrementCurrentIndex();
        }
    }

    CarouselNavigationButton {
        Kirigami.Theme.inherit: true
        LayoutMirroring.enabled: root.LayoutMirroring.enabled

        animated: root.animated
        atBeginning: root.atBeginning
        atEnd: root.atEnd
        edgeMargin: root.edgeMargin
        role: Qt.AlignTrailing

        onClicked: {
            root.incrementCurrentIndex();
        }
    }
}
