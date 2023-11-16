/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

Kirigami.LinkButton {
    id: root

    property Kirigami.OverlaySheet __overlay

    function __open() {
        if (!__overlay) {
            __overlay = overlaySheetComponent.createObject(this);
        }
        __overlay.open();
    }

    text: i18nd("libdiscover", "Show Dependencies…")

    visible: dependenciesModel.count > 0

    onClicked: __open()

    Connections {
        target: resource
        function onDependenciesFound(dependencies) {
            dependenciesModel.clear()
            for (const dependency of dependencies) {
                dependenciesModel.append(dependency)
            }
        }
    }

    ListModel {
        id: dependenciesModel
    }

    Component {
        id: overlaySheetComponent

        Kirigami.OverlaySheet {
            parent: root.T.Overlay.overlay

            title: i18nd("libdiscover", "Dependencies for package: %1", resource.packageName)

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

                model: dependenciesModel

                section.property: "packageInfo"
                section.delegate: Kirigami.ListSectionHeader {
                    required property string section

                    width: ListView.view.width
                    text: section
                }

                delegate: KD.SubtitleDelegate {
                    required property string packageName
                    required property string packageDescription

                    width: ListView.view.width
                    icon.width: 0

                    text: packageName
                    subtitle: packageDescription

                    // No need to offer a hover/selection effect since these list
                    // items are non-interactive and non-selectable
                    hoverEnabled: false
                    down: false
                }
            }
        }
    }
}
