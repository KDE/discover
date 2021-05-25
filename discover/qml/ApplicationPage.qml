/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.12
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.14 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo
    property QtObject application: null
    readonly property int visibleReviews: 3
    title: appInfo.application.name
    clip: true

    // Usually this page is not the top level page, but when we are, isHome being
    // true will ensure that the search field suggests we are searching in the list
    // of available apps, not inside the app page itself. This will happen when
    // Discover is launched e.g. from krunner or otherwise requested to show a
    // specific application on launch.
    readonly property bool isHome: true
    function searchFor(text) {
        if (text.length === 0)
            return;
        Navigation.openCategory(null, "")
    }

    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false
    }

    ReviewsPage {
        id: reviewsSheet
        model: ReviewsModel {
            id: reviewsModel
            resource: appInfo.application
        }
    }

    contextualActions: [originsMenuAction]

    ActionGroup {
        id: sourcesGroup
        exclusive: true
    }

    Kirigami.Action {
        id: originsMenuAction

        text: i18n("Sources")
        visible: children.length>1
        children: sourcesGroup.actions
        readonly property var r0: Instantiator {
            model: ResourcesProxyModel {
                id: alternativeResourcesModel
                allBackends: true
                resourcesUrl: appInfo.application.url
            }
            delegate: Action {
                ActionGroup.group: sourcesGroup
                text: model.application.availableVersion ? i18n("%1 - %2", displayOrigin, model.application.availableVersion) : displayOrigin
                icon.name: sourceIcon
                checkable: true
                checked: appInfo.application === model.application
                onTriggered: if(index>=0) {
                    appInfo.application = model.application
                }
            }
        }
    }

    Kirigami.Action {
        id: invokeAction
        visible: application.isInstalled && application.canExecute && !appbutton.isActive
        text: application.executeLabel
        icon.name: "media-playback-start"
        onTriggered: application.invokeApplication()
    }

    actions {
        main: appbutton.action
        right: appbutton.isActive ? appbutton.cancelAction : invokeAction
    }

    InstallApplicationButton {
        id: appbutton
        Layout.rightMargin: Kirigami.Units.smallSpacing
        application: appInfo.application
        visible: false
    }

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    // Icon, name, caption, screenshots, description and reviews
    ColumnLayout {
        spacing: 0
        Kirigami.FlexColumn {
            Layout.fillWidth: true
            maximumWidth: Kirigami.Units.gridUnit * 40
            RowLayout {
                Layout.margins: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    Layout.preferredHeight: 80
                    Layout.preferredWidth: 80
                    source: appInfo.application.icon
                    Layout.rightMargin: Kirigami.Units.smallSpacing * 2
                }
                ColumnLayout {
                    spacing: 0
                    Kirigami.Heading {
                        text: appInfo.application.name
                        lineHeight: 1.0
                        maximumLineCount: 1
                        font.weight: Font.DemiBold
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
                        Label {
                            text: appInfo.application.rating ? i18np("%1 rating", "%1 ratings", appInfo.application.rating.ratingCount) : i18n("No ratings yet")
                            opacity: 0.6
                        }
                    }
                    Kirigami.Heading {
                        id: summary
                        level: 4
                        text: appInfo.application.author
                        opacity: 0.7
                        maximumLineCount: 2
                        lineHeight: lineCount > 1 ? 0.75 : 1.2
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                    }
                }
            }
        }

        ColumnLayout {
            Layout.topMargin: Kirigami.Units.gridUnit * 2
            Layout.fillWidth: true
            ApplicationScreenshots {
                id: applicationScreenshots
                visible: count > 0
                resource: appInfo.application
                ScrollBar.horizontal: screenshotsScrollbar
                Layout.bottomMargin: Kirigami.Units.gridUnit * 2
                Layout.preferredHeight: Math.min(Kirigami.Units.gridUnit * 20, Window.height * 0.4)
                Layout.fillWidth: true
            }
            ScrollBar {
                id: screenshotsScrollbar
                Layout.fillWidth: true
                visible: applicationScreenshots.visible
            }
        }

        Kirigami.FlexColumn {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            maximumWidth: Kirigami.Units.gridUnit * 40
            Kirigami.Heading {
                text: appInfo.application.comment
                level: 2
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: appInfo.application.longDescription
                onLinkActivated: Qt.openUrlExternally(link);
            }

            Kirigami.Heading {
                Layout.topMargin: visible ? Kirigami.Units.largeSpacing : 0
                text: i18n("What's New")
                level: 2
                visible: changelogLabel.text.length > 0
            }

            Label {
                id: changelogLabel
                Layout.topMargin: text.length > 0 ? Kirigami.Units.largeSpacing : 0
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                Component.onCompleted: appInfo.application.fetchChangelog()
                Connections {
                    target: appInfo.application
                    function onChangelogFetched(changelog) {
                        changelogLabel.text = changelog
                    }
                }
            }

            Kirigami.LinkButton {
                id: addonsButton
                text: i18n("Addons")
                visible: addonsView.containsAddons
                onClicked: if (addonsView.addonsCount === 0) {
                    Navigation.openExtends(application.appstreamId)
                } else {
                    addonsView.sheetOpen = true
                }
            }

            Kirigami.Heading {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                font.weight: Font.DemiBold
                text: i18n("Reviews")
                Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                level: 2
                visible: rep.count > 0
            }

            Repeater {
                id: rep
                model: PaginateModel {
                    sourceModel: reviewsSheet.model
                    pageSize: visibleReviews
                }
                delegate: ReviewDelegate {
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true
                    separator: false
                    compact: true
                }
            }

            RowLayout {
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing

                Button {
                    visible: reviewsModel.count > visibleReviews

                    text: i18np("Show %1 Review…", "Show All %1 Reviews…", reviewsModel.count)
                    icon.name: "view-visible"

                    onClicked: {
                        reviewsSheet.open()
                    }
                }

                Button {
                    visible: appbutton.isStateAvailable && reviewsModel.backend && reviewsModel.backend.isResourceSupported(appInfo.application)
                    enabled: appInfo.application.isInstalled

                    text: appInfo.application.isInstalled ? i18n("Write a Review") : i18n("Install to Write a Review")
                    icon.name: "document-edit"

                    onClicked: {
                        reviewsSheet.openReviewDialog()
                    }
                }
            }

            Repeater {
                model: application.objects
                delegate: Loader {
                    property QtObject resource: appInfo.application
                    source: modelData
                }
            }


            // Details/metadata
            Kirigami.Separator {
                Layout.fillWidth: true
                Layout.bottomMargin: Kirigami.Units.largeSpacing
            }

            Kirigami.FormLayout {
                Layout.fillWidth: true

                // Category row
                Label {
                    Kirigami.FormData.label: i18n("Category:")
                    visible: text.length > 0
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: appInfo.application.categoryDisplay
                }

                // Version row
                Label {
                    readonly property string version: appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion
                    readonly property string releaseDate: appInfo.application.releaseDate.toLocaleDateString(Locale.ShortFormat)

                    function versionString() {
                        if (version.length == 0) {
                            return ""
                        } else {
                            if (releaseDate.length > 0) {
                                return i18n("%1, released on %2", version, releaseDate)
                            } else {
                                return version
                            }
                        }
                    }

                    Kirigami.FormData.label: i18n("Version:")
                    visible: text.length > 0
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: versionString()
                }

                // Author row
                Label {
                    Kirigami.FormData.label: i18n("Author:")
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    visible: text.length>0
                    text: appInfo.application.author
                }

                // Size row
                Label {
                    Kirigami.FormData.label: i18n("Size:")
                    Layout.fillWidth: true
                    Layout.alignment: Text.AlignTop
                    elide: Text.ElideRight
                    text: appInfo.application.sizeDescription
                }

                // Source row
                Label {
                    Kirigami.FormData.label: i18n("Source:")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    text: appInfo.application.displayOrigin
                    elide: Text.ElideRight
                }

                // License row
                RowLayout {
                    Kirigami.FormData.label: i18n("License:")
                    visible: appInfo.application.licenses.length>0
                    Layout.fillWidth: true
                    Repeater {
                        model: appInfo.application.licenses
                        delegate: Kirigami.UrlButton {
                            id: licenseButton
                            horizontalAlignment: Text.AlignLeft
                            ToolTip.text: i18n("See full license terms")
                            ToolTip.visible: licenseButton.mouseArea.containsMouse
                            text: modelData.name
                            url: modelData.url
                            enabled: url !== ""
                        }
                    }
                }

                // "User Guide" row
                Kirigami.UrlButton {
                    Kirigami.FormData.label: i18n ("Documentation:")
                    text: i18n("Read the user guide")
                    url: application.helpURL
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }

                // Homepage row
                Kirigami.UrlButton {
                    Kirigami.FormData.label: i18n("Get involved:")
                    text: i18n("Visit the app's website")
                    url: application.homepage
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }

                // Donate row
                Kirigami.UrlButton {
                    text: i18n("Make a donation")
                    url: application.donationURL
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }

                // "Report a Problem" row
                Kirigami.UrlButton {
                    text: i18n("Report a problem")
                    url: application.bugURL
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
    }
}
