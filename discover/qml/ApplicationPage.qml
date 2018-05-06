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
import QtQuick.Controls 2.3 as QQC2
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.1 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo
    property QtObject application: null
    clip: true

    background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

    ReviewsPage {
        id: reviewsSheet
        model: ReviewsModel {
            id: reviewsModel
            resource: appInfo.application
        }
    }

    contextualActions: [originsMenuAction]

    QQC2.ActionGroup {
        id: sourcesGroup
        exclusive: true
    }

    Kirigami.Action {
        id: originsMenuAction

        text: i18n("Sources")
        visible: children.length>1
        readonly property var r0: Instantiator {
            model: ResourcesProxyModel {
                id: alternativeResourcesModel
                allBackends: true
                resourcesUrl: appInfo.application.url
            }
            delegate: QQC2.Action {
                QQC2.ActionGroup.group: sourcesGroup
                text: displayOrigin
                icon.name: sourceIcon
                checked: appInfo.application == model.application
                onTriggered: if(index>=0) {
                    var res = model.application
                    console.assert(res)
                    window.stack.pop()
                    Navigation.openApplication(res)
                }
            }
            onObjectAdded: originsMenuAction.children.push(object)
        }
    }

    actions {
        main: appbutton.action
        right: Kirigami.Action {
            visible: application.isInstalled && application.canExecute
            text: application.executeLabel
            icon.name: "media-playback-start"
            onTriggered: application.invokeApplication()
        }
    }

    InstallApplicationButton {
        id: appbutton
        Layout.rightMargin: Kirigami.Units.smallSpacing
        application: appInfo.application
        visible: false
    }

    leftPadding: Kirigami.Units.largeSpacing * (applicationWindow().wideScreen ? 2 : 1)
    rightPadding: Kirigami.Units.largeSpacing * (applicationWindow().wideScreen ? 2 : 1)
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
                RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Rating {
                        rating: appInfo.application.rating ? appInfo.application.rating.sortableRating : 0
                        starSize: summary.font.pointSize
                    }
                    QQC2.Label {
                        text: appInfo.application.rating ? i18np("%1 rating", "%1 ratings", appInfo.application.rating.ratingCount) : i18n("No ratings yet")
                        opacity: 0.5
                    }
                }
                Kirigami.Heading {
                    id: summary
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
            id: screenshots
            Layout.fillWidth: true
            visible: count > 0
            resource: appInfo.application
            QQC2.ScrollBar.horizontal: screenshotsScrollbar
        }
        QQC2.ScrollBar {
            id: screenshotsScrollbar
            Layout.fillWidth: true
        }

        QQC2.Label {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: appInfo.application.longDescription
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            text: i18n("What's New")
            level: 2
            visible: changelogLabel.text.length > 0
        }

        Rectangle {
            color: Kirigami.Theme.linkColor
            Layout.fillWidth: true
            height: 1
            visible: changelogLabel.text.length > 0
        }

        QQC2.Label {
            id: changelogLabel
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            Component.onCompleted: appInfo.application.fetchChangelog()
            Connections {
                target: appInfo.application
                onChangelogFetched: {
                    changelogLabel.text = changelog
                }
            }
        }


        LinkButton {
            id: addonsButton
            text: i18n("Addons")
            visible: addonsView.containsAddons
            onClicked: addonsView.sheetOpen = true
        }


        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            text: i18n("Reviews")
            level: 2
            visible: rep.count > 0
        }

        Rectangle {
            color: Kirigami.Theme.linkColor
            Layout.fillWidth: true
            height: 1
            visible: rep.count > 0
        }

        Repeater {
            id: rep
            model: PaginateModel {
                sourceModel: reviewsSheet.model
                pageSize: 3
            }
            delegate: ReviewDelegate {
                Layout.topMargin: Kirigami.Units.largeSpacing
                separator: false
                compact: true
                Layout.bottomMargin: Kirigami.Units.largeSpacing
            }
        }
        LinkButton {
            text: appInfo.application.isInstalled? i18n("Be the first to write a review!") : i18n("Install this app and be the first to write a review!")
            onClicked: reviewsSheet.openReviewDialog()
            enabled: appInfo.application.isInstalled
            visible: !commentsButton.visible && reviewsModel.backend && reviewsModel.backend.isResourceSupported(appInfo.application)
        }
        LinkButton {
            id: commentsButton
            visible: reviewsModel.count > 0
            text: i18n("Show all %1 reviews...", reviewsModel.count)

            onClicked: {
                reviewsSheet.open()
            }
            Layout.bottomMargin: Kirigami.Units.largeSpacing
        }

        Repeater {
            model: application.objects
            delegate: Loader {
                property QtObject resource: appInfo.application
                source: modelData
            }
        }

        Item {
            height: addonsButton.height
            width: 5
        }

        // Details/metadata
        Rectangle {
            color: Kirigami.Theme.linkColor
            Layout.fillWidth: true
            height: 1
            Layout.bottomMargin: Kirigami.Units.largeSpacing
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
                visible: versionLabel.visible
                Layout.alignment: Qt.AlignRight
                text: i18n("Version:")
            }
            QQC2.Label {
                readonly property string version: appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion
                id: versionLabel
                visible: text.length > 0
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
                text: appInfo.application.sizeDescription
            }

            // Source row
            QQC2.Label {
                Layout.alignment: Qt.AlignRight
                text: i18n("Source:")
            }
            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: appInfo.application.displayOrigin
                elide: Text.ElideRight
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
                visible: homepageLink.visible
                Layout.alignment: Qt.AlignRight
                text: i18n("Homepage:")
            }
            LinkButton {
                id: homepageLink
                visible: text.length > 0
                text: application.homepage
                onClicked: Qt.openUrlExternally(application.homepage)
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }

            // "User Guide" row
            QQC2.Label {
                visible: docsLink.visible
                Layout.alignment: Qt.AlignRight
                text: i18n("User Guide:")
            }
            LinkButton {
                id: docsLink
                visible: text.length > 0
                text: application.helpURL
                onClicked: Qt.openUrlExternally(helpURL)
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }

            // Donate row
            QQC2.Label {
                visible: donationLink.visible
                Layout.alignment: Qt.AlignRight
                text: i18n("Donate:")
            }
            LinkButton {
                id: donationLink
                visible: text.length > 0
                text: application.donationURL
                onClicked: Qt.openUrlExternally(donationURL)
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }

            // "Report a Droblem" row
            QQC2.Label {
                visible: bugLink.visible
                Layout.alignment: Qt.AlignRight
                text: i18n("Report a Problem:")
            }
            LinkButton {
                id: bugLink
                visible: text.length > 0
                text: application.bugURL
                onClicked: Qt.openUrlExternally(bugURL)
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
