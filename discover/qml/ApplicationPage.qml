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

import QtQuick 2.5
import QtQuick.Controls 1.1
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.1 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo
    property QtObject application: null
    clip: true

    title: application.name

    background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

    ReviewsPage {
        id: reviewsSheet
        model: ReviewsModel {
            id: reviewsModel
            resource: appInfo.application
        }
    }

    Kirigami.OverlaySheet {
        id: originsOverlay
        bottomPadding: Kirigami.Units.largeSpacing
        topPadding: Kirigami.Units.largeSpacing
        readonly property alias model: alternativeResourcesView.model
        function listBackends() {
            var first = true;
            var ret = "";
            var m = alternativeResourcesView.model;
            for(var i=0, count=m.rowCount(); i<count; ++i) {
                var res = m.resourceAt(i)
                if (res != appInfo.application) {
                    if (!first) {
                        ret += ", "
                    }
                    ret += "<a href='" + i + "'>" + res.displayOrigin + "</a>"
                    first = false
                }
            }
            return ret
        }
        readonly property string sentence: alternativeResourcesView.count <= 1 ? "" : i18n("\nAlso available in %1", listBackends())
        ListView {
            id: alternativeResourcesView
            model: ResourcesProxyModel {
                allBackends: true
                resourcesUrl: appInfo.application.url
            }
            delegate: Kirigami.BasicListItem {
                label: displayOrigin
                checked: appInfo.application == model.application
                onClicked: if(index>=0) {
                    var res = model.application
                    console.assert(res)
                    window.stack.pop()
                    Navigation.openApplication(res)
                }
            }
        }
    }

    header: QQC2.ToolBar {
        anchors {
            right: parent.right
            left: parent.left
        }

        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            ToolButton {
                Layout.leftMargin: Kirigami.Units.smallSpacing
                iconName: "draw-arrow-back"
                tooltip: i18n("Back")
                enabled: appInfo.sClose.enabled
                onClicked: appInfo.sClose.activated()
            }
            Item {
                Layout.fillWidth: true
            }
            Kirigami.Heading {
                level: 3
                Layout.maximumWidth: parent.width/2
                text: appInfo.application.name
                maximumLineCount: 1
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }
            Item {
                Layout.fillWidth: true
            }

            Binding {
                target: appInfo.actions
                property: "main"
                value: appbutton.action
            }

            InstallApplicationButton {
                id: appbutton
                Layout.rightMargin: Kirigami.Units.smallSpacing
                application: appInfo.application
                visible: applicationWindow().wideScreen
            }

            Button {
                Layout.rightMargin: Kirigami.Units.smallSpacing
                visible: application.isInstalled && application.canExecute
                text: i18n("Launch")
                onClicked: application.invokeApplication()
            }
        }
    }

    // Icon, name, caption, screenshots, description and reviews
    ColumnLayout {
        spacing: 0
        RowLayout {
            Kirigami.Icon {
                Layout.preferredHeight: 80
                Layout.preferredWidth: 80
                source: appInfo.application.icon
                Layout.rightMargin: Kirigami.Units.smallSpacing * 2
            }
            ColumnLayout {
                spacing: 0
                Kirigami.Heading {
                    level: 1
                    text: appInfo.application.name
                    lineHeight: 1.0
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.alignment: Text.AlignBottom
                }
                Kirigami.Heading {
                    level: 4
                    text: appInfo.application.comment
                    maximumLineCount: 2
                    lineHeight: lineCount > 1 ? 0.75 : 1.2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
            }
            Layout.bottomMargin: Kirigami.Units.largeSpacing
        }

        ApplicationScreenshots {
            Layout.fillWidth: true
            resource: appInfo.application
            page: appInfo
            Layout.bottomMargin: Kirigami.Units.largeSpacing
        }

        QQC2.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignJustify
            wrapMode: Text.WordWrap
            text: appInfo.application.longDescription + originsOverlay.sentence
            onLinkActivated: {
                var idx = parseInt(link, 10)
                var res = originsOverlay.model.resourceAt(idx)
                window.stack.pop()
                Navigation.openApplication(res)
            }
        }

        LinkButton {
            id: addonsButton
            text: i18n("Addons")
            visible: addonsView.containsAddons
            onClicked: addonsView.sheetOpen = true
        }

        LinkButton {
            text: i18n("Review")
            onClicked: reviewsSheet.openReviewDialog()
            visible: !commentsButton.visible && reviewsModel.backend && reviewsModel.backend.isResourceSupported(appInfo.application)
        }
        LinkButton {
            id: commentsButton
            readonly property QtObject rating: appInfo.application.rating
            visible: rating && rating.ratingCount>0 && reviewsModel.count
            text: i18n("Show reviews (%1)...", rating ? reviewsModel.count : 0)

            onClicked: {
                reviewsSheet.open()
            }
        }

        Item {
            height: addonsButton.height
            width: 5
        }

        // Details/metadata
        Rectangle {
            Layout.topMargin: Kirigami.Units.smallSpacing
            color: Kirigami.Theme.linkColor
            Layout.fillWidth: true
            height: 1
            Layout.bottomMargin: Kirigami.Units.smallSpacing
        }
        GridLayout {
            rowSpacing: 0
            columns: 2

            // Category row
            QQC2.Label {
                Layout.alignment: Qt.AlignRight
                text: i18n("Category:")
            }
            QQC2.Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: appInfo.application.categoryDisplay
            }

            // Version row
            QQC2.Label {
                readonly property string version: appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion
                visible: version.length > 0
                Layout.alignment: Qt.AlignRight
                text: i18n("Version:")
            }
            QQC2.Label {
                readonly property string version: appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion
                visible: version.length > 0
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: version ? version : ""
            }

            // Size row
            QQC2.Label {
                Layout.alignment: Qt.AlignRight
                text: i18n("Size:")
            }
            QQC2.Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: i18n("%1", appInfo.application.sizeDescription)
            }

            // Source row
            QQC2.Label {
                Layout.alignment: Qt.AlignRight
                text: i18n("Source:")
            }
            LinkButton {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                enabled: alternativeResourcesView.count > 1
                text: appInfo.application.displayOrigin
                elide: Text.ElideRight
                onClicked: originsOverlay.open()
            }

            // License row
            QQC2.Label {
                Layout.alignment: Qt.AlignRight
                text: i18n("License:")
                visible: appInfo.application.license.length>0
            }
            LinkButton {
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                visible: text.length>0
                text: appInfo.application.license
//                         tooltip: i18n("See full license terms")
                onClicked: Qt.openUrlExternally("https://spdx.org/licenses/" + appInfo.application.license + ".html#licenseText")
            }

            // Homepage row
            QQC2.Label {
                readonly property string homepage: application.homepage
                visible: homepage.length > 0
                Layout.alignment: Qt.AlignRight
                text: i18n("Homepage:")
            }
            LinkButton {
                readonly property string homepage: application.homepage
                visible: homepage.length > 0
                text: homepage
                onClicked: Qt.openUrlExternally(application.homepage)
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
        parent: overlay
    }
}
