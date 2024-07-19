/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

Kirigami.LinkButton {
    id: root

    // actual subtype: PackageKitResource
    required property Discover.AbstractResource resource
    Discover.Activatable.active: resource.dependencies.length > 0

    property Kirigami.OverlaySheet __overlay

    function __open() {
        if (!__overlay) {
            __overlay = overlaySheetComponent.createObject(this);
        }
        __overlay.open();
    }

    text: i18nd("libdiscover", "Show Dependencies…")

    onClicked: __open()

    Component {
        id: overlaySheetComponent

        Kirigami.OverlaySheet {
            parent: root.T.Overlay.overlay

            title: i18nd("libdiscover", "Dependencies for package: %1", root.resource.packageName)

            focus: true

            ListView {
                id: view

                implicitWidth: Kirigami.Units.gridUnit * 26

                // During initialization and initial section and row delegates
                // creation, contentHeight may vary wildly. And it is also a
                // subject to changes when scrolling away from a
                // differently-sized section delegate. Meanwhile, if the
                // height changes, the ListView would reset scroll position
                // to top, which is not only inconvenient but also in turn
                // affects contentHeight estimations again. So we only let it
                // bind until Component.completed hook: since the list isn't
                // expected to change at runtime, we should not worry about
                // layout size changes on the fly.
                implicitHeight: contentHeight

                Component.onCompleted: {
                    // Break the binding
                    implicitHeight = implicitHeight;
                }

                model: root.resource.dependencies

                section.property: "infoString"
                section.delegate: Kirigami.ListSectionHeader {
                    required property string section

                    width: ListView.view.width
                    text: section
                }

                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property var modelData

                    readonly property string summary: modelData.summary

                    width: ListView.view.width
                    icon.width: 0

                    text: modelData.packageName

                    // No need to offer a hover/selection effect since these list
                    // items are non-interactive and non-selectable
                    hoverEnabled: false
                    down: false

                    // ToolTip is intentionally omitted, as everything is wrapped and thus visible

                    contentItem: KD.IconTitleSubtitle {
                        icon: icon.fromControlsIcon(delegate.icon)
                        title: delegate.text
                        subtitle: delegate.summary
                        selected: delegate.highlighted || delegate.down
                        font: delegate.font
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
