/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.19 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo

    title: appInfo.application.name
    clip: true

    property QtObject application: null
    readonly property int visibleReviews: 3
    readonly property int internalSpacings: Kirigami.Units.largeSpacing
    readonly property int pageContentMargins: Kirigami.Units.gridUnit

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
        main: appbutton.isActive ? appbutton.cancelAction : appbutton.action
        right: invokeAction
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

    // Page content
    ColumnLayout {
        id: pageLayout

        spacing: appInfo.internalSpacings

        // Header and its background rectangle
        Rectangle {
            Layout.fillWidth: true

            implicitHeight: headerLayout.height + (headerLayout.anchors.topMargin * 2)
            color: Qt.darker(Kirigami.Theme.backgroundColor, 1.05)

            // Header layout with App icon, name, author, rating,
            // screenshots, and metadata
            ColumnLayout {
                id: headerLayout

                anchors {
                    topMargin: appInfo.internalSpacings
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }

                spacing: appInfo.internalSpacings

                // App icon, name, author, rating
                RowLayout {
                    Layout.leftMargin: appInfo.internalSpacings
                    Layout.rightMargin: appInfo.internalSpacings

                    spacing: 0 // Children bring their own

                    // App icon
                    Kirigami.Icon {
                        readonly property int size : applicationWindow().wideScreen ? Kirigami.Units.iconSizes.large * 2 : Kirigami.Units.iconSizes.huge

                        implicitWidth: size
                        implicitHeight: size
                        source: appInfo.application.icon
                        Layout.rightMargin: appInfo.internalSpacings
                    }

                    // App name, author, and rating
                    ColumnLayout {
                        Layout.fillWidth: true

                        spacing: 0

                        // App name
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            text: appInfo.application.name
                            font.weight: Font.DemiBold
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }

                        // Author
                        Label {
                            id: author
                            Layout.fillWidth: true
                            visible: text.length > 0
                            opacity: 0.8
                            text: appInfo.application.author
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }

                        Item {
                            implicitHeight: appInfo.internalSpacings
                        }

                        // Rating
                        RowLayout {
                            Layout.fillWidth: true

                            Rating {
                                opacity: 0.8
                                rating: appInfo.application.rating ? appInfo.application.rating.sortableRating : 0
                                starSize: author.font.pointSize
                            }

                            Label {
                                opacity: 0.8
                                text: appInfo.application.rating ? i18np("%1 rating", "%1 ratings", appInfo.application.rating.ratingCount) : i18n("No ratings yet")
                            }
                        }
                    }
                }

                // Screenshots
                ScrollView {
                    id: screenshotsScroll
                    visible: screenshots.count > 0
                    Layout.maximumWidth: headerLayout.width
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: Math.min(Kirigami.Units.gridUnit * 20, Window.height * 0.25)

                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                    ApplicationScreenshots {
                        id: screenshots
                        resource: appInfo.application
                        delegateHeight: parent.Layout.preferredHeight * 0.8
                        showNavigationArrows: screenshotsScroll.width === headerLayout.width
                    }
                }

                // Metadata
                Flow {
                    id: metadataLayout
                    readonly property int itemWidth: Kirigami.Units.gridUnit * 6

                    Layout.leftMargin: appInfo.internalSpacings
                    Layout.rightMargin: appInfo.internalSpacings
                    // This centers the Flow in the page, no matter how many items have
                    // flowed onto other rows
                    Layout.maximumWidth: ((itemWidth + spacing) * Math.min(children.length, Math.floor(headerLayout.width / (itemWidth + spacing)))) - spacing
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    spacing: appInfo.internalSpacings

                    // Version
                    ColumnLayout {
                        id: versionColumn
                        width: metadataLayout.itemWidth

                        spacing: Kirigami.Units.smallSpacing

                        Label {
                            Layout.fillWidth: true
                            opacity: 0.7
                            text: i18n("Version")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                        Label {
                            Layout.fillWidth: true
                            text: appInfo.application.versionString
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    // Size
                    ColumnLayout {
                        id: sizeColumn
                        width: metadataLayout.itemWidth

                        spacing: Kirigami.Units.smallSpacing

                        Label {
                            Layout.fillWidth: true
                            opacity: 0.7
                            text: i18n("Size")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                        Label {
                            Layout.fillWidth: true
                            text: appInfo.application.sizeDescription
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    // Distributor
                    ColumnLayout {
                        id: distributorColumn
                        width: metadataLayout.itemWidth

                        spacing: Kirigami.Units.smallSpacing

                        Label {
                            Layout.fillWidth: true
                            opacity: 0.7
                            text: i18n("Distributed by")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                        Label {
                            Layout.fillWidth: true
                            text: appInfo.application.displayOrigin
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    // License(s)
                    ColumnLayout {
                        id: licenseColumn
                        width: metadataLayout.itemWidth

                        spacing: Kirigami.Units.smallSpacing

                        Label {
                            Layout.fillWidth: true
                            opacity: 0.7
                            text: i18np("License", "Licenses", appInfo.application.licenses.length)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                        Label {
                            Layout.fillWidth: true
                            visible : appInfo.application.licenses.length === 0
                            text: i18nc("The app does not provide any licenses", "Unknown")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible : appInfo.application.licenses.length > 0
                            spacing: 0

                            Repeater {
                                model: appInfo.application.licenses.slice(0, 2)
                                delegate: Kirigami.UrlButton {
                                    Layout.fillWidth: true
                                    enabled: url !== ""
                                    text: modelData.name
                                    url: modelData.url
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignTop
                                    elide: Text.ElideRight
                                }
                            }

                            Kirigami.LinkButton {
                                Layout.fillWidth: true
                                visible: application.licenses.length > 3
                                text: i18np("See more…", "See more…", appInfo.application.licenses.length)
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignTop
                                elide: Text.ElideRight
                                onClicked: allLicensesSheet.open();
                            }
                        }
                    }
                }
            }

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.bottom
                }
            }
        }


        Repeater {
            model: application.topObjects
            delegate: Loader {
                property QtObject resource: appInfo.application

                Layout.fillWidth: item.Layout.fillWidth
                Layout.leftMargin: appInfo.pageContentMargins
                Layout.rightMargin: appInfo.pageContentMargins

                source: modelData
            }
        }

        // Layout for textual content; this isn't in the main ColumnLayout
        // because we want it to be bounded to a maximum width
        ColumnLayout {
            id: textualContentLayout

            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 35
            Layout.margins: appInfo.pageContentMargins
            Layout.alignment: Qt.AlignHCenter

            spacing: appInfo.internalSpacings

            // Short description
            Kirigami.Heading {
                Layout.fillWidth: true
                level: 2
                font.weight: Font.DemiBold
                text: appInfo.application.comment
                wrapMode: Text.Wrap
                maximumLineCount: 5
                elide: Text.ElideRight
            }

            // Long app description
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: appInfo.application.longDescription
                textFormat: Text.StyledText
                onLinkActivated: Qt.openUrlExternally(link);
            }

            // External resources
            Flow {
                id: buttonLayout
                readonly property int widestButton: Math.max(helpButton.implicitWidth,
                                                             homepageButton.implicitWidth,
                                                             donateButton.implicitWidth,
                                                             addonsButton.implicitWidth,
                                                             bugButton.implicitWidth)
                readonly property int visibleButtons: (helpButton.visible ? 1 : 0) +
                (homepageButton.visible ? 1: 0) +
                (donateButton.visible ? 1 : 0) +
                (addonsButton.visible ? 1 : 0) +
                (bugButton.visible ? 1 : 0)

                // This centers the Flow in the page, no matter how many items have
                // flowed onto other rows
                Layout.maximumWidth: widestButton * Math.min(visibleButtons, Math.floor(textualContentLayout.width / widestButton))
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                visible: visibleButtons > 0

                spacing: 0 // Need to save horizontal space

                ToolButton {
                    id: helpButton
                    width: buttonLayout.widestButton
                    visible: application.helpURL != ""
                    display: ToolButton.TextUnderIcon
                    text: i18n("Documentation")
                    icon.name: "documentation"
                    onClicked: Qt.openUrlExternally(application.helpURL);

                    ToolTip {
                        text: application.helpURL
                    }
                }

                ToolButton {
                    id: homepageButton
                    width: buttonLayout.widestButton
                    visible: application.homepage != ""
                    display: ToolButton.TextUnderIcon
                    text: i18n("Website")
                    icon.name: "internet-services"
                    onClicked: Qt.openUrlExternally(application.homepage);

                    ToolTip {
                        text: application.homepage
                    }
                }

                ToolButton {
                    id: donateButton
                    width: buttonLayout.widestButton
                    visible: application.donationURL != ""
                    display: ToolButton.TextUnderIcon
                    text: i18n("Donate")
                    icon.name: "help-donate"
                    onClicked: Qt.openUrlExternally(application.donationURL);

                    ToolTip {
                        text: application.donationURL
                    }
                }

                ToolButton {
                    id: addonsButton
                    width: buttonLayout.widestButton
                    visible: addonsView.containsAddons
                    display: ToolButton.TextUnderIcon
                    text: i18n("Addons")
                    icon.name: "extension-symbolic"
                    onClicked: if (addonsView.addonsCount === 0) {
                        Navigation.openExtends(application.appstreamId, appInfo.application.name)
                    } else {
                        addonsView.sheetOpen = true
                    }

                    ToolTip {
                        text: i18n("Install or remove add-ons for %1", appInfo.application.name)
                    }
                }

                ToolButton {
                    id: bugButton
                    width: buttonLayout.widestButton
                    visible: application.bugURL != ""
                    display: ToolButton.TextUnderIcon
                    text: i18n("Report Bug")
                    icon.name: "tools-report-bug"
                    onClicked: Qt.openUrlExternally(application.bugURL);

                    ToolTip {
                        text: application.bugURL
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
                visible: buttonLayout.visible
            }

            Kirigami.Heading {
                text: i18n("What's New")
                level: 2
                font.weight: Font.DemiBold
                visible: changelogLabel.visible
            }

            // Changelog text
            Label {
                id: changelogLabel
                Layout.fillWidth: true
                visible: text.length > 0
                wrapMode: Text.WordWrap

                Component.onCompleted: appInfo.application.fetchChangelog()
                Connections {
                    target: appInfo.application
                    function onChangelogFetched(changelog) {
                        changelogLabel.text = changelog
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
                visible: changelogLabel.visible
            }

            Kirigami.Heading {
                Layout.fillWidth: true
                visible: rep.count > 0
                font.weight: Font.DemiBold
                text: i18n("Reviews")
                level: 2
            }

            // Top three reviews
            Repeater {
                id: rep
                model: PaginateModel {
                    sourceModel: reviewsSheet.model
                    pageSize: visibleReviews
                }
                delegate: ReviewDelegate {
                    Layout.fillWidth: true
                    separator: false
                    compact: true
                }
            }

            // Review-related buttons
            Flow {
                Layout.fillWidth: true

                spacing: appInfo.internalSpacings

                Button {
                    visible: reviewsModel.count > visibleReviews

                    text: i18np("Show all %1 Reviews", "Show all %1 Reviews", reviewsModel.count)
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
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
    }

    Kirigami.OverlaySheet {
        id: allLicensesSheet
        parent: applicationWindow().overlay

        title: i18n("All Licenses")


        ListView {
            id: listview

            implicitWidth: Kirigami.Units.gridUnit

            model: appInfo.application.licenses

            delegate: Kirigami.BasicListItem {
                activeBackgroundColor: "transparent"
                activeTextColor: Kirigami.Theme.textColor
                separatorVisible: false
                contentItem: Kirigami.UrlButton {
                    enabled: url !== ""
                    text: modelData.name
                    url: modelData.url
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }
    }
}
