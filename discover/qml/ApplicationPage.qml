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
import org.kde.kirigami 2.20 as Kirigami
import org.kde.purpose 1.0 as Purpose
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

    readonly property bool isOfflineUpgrade: application.packageName === "discover-offline-upgrade"

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

                // App icon, name, author or update info, rating
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
                            type: Kirigami.Heading.Type.Primary
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }

                        // Author (for apps) or upgrade info (for offline upgrades)
                        Label {
                            id: author

                            Layout.fillWidth: true
                            Layout.bottomMargin: appInfo.internalSpacings

                            visible: text.length > 0

                            opacity: 0.8
                            text: {
                                if (appInfo.isOfflineUpgrade) {
                                    return appInfo.application.upgradeText.length > 0 ? appInfo.application.upgradeText : "";
                                } else if (appInfo.application.author.length > 0) {
                                    return appInfo.application.author;
                                } else {
                                    return i18n("Unknown author");
                                }
                            }
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }

                        // Rating
                        RowLayout {
                            Layout.fillWidth: true

                            // Not relevant to the offline upgrade use case
                            visible: !appInfo.isOfflineUpgrade

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
                Kirigami.InlineMessage {
                    type: Kirigami.MessageType.Warning
                    Layout.fillWidth: true
                    Layout.margins: Kirigami.Units.smallSpacing
                    visible: screenshots.hasFailed
                    text: i18n("Could not access the screenshots")
                }

                ScrollView {
                    id: screenshotsScroll
                    visible: screenshots.count > 0 && !screenshots.hasFailed
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
                    readonly property int itemWidth: Kirigami.Units.gridUnit * 7
                    readonly property int visibleChildren: countVisibleChildren(children)
                    function countVisibleChildren(items) {
                        let ret = 0;
                        for (const itemPos in items) {
                            const item = items[itemPos];
                            ret += item.visible;
                        }
                        return ret;
                    }

                    // This centers the Flow in the page, no matter how many items have
                    // flowed onto other rows
                    Layout.maximumWidth: ((itemWidth + spacing) * Math.min(visibleChildren, Math.floor(headerLayout.width / (itemWidth + spacing)))) - spacing
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: appInfo.internalSpacings
                    Layout.rightMargin: appInfo.internalSpacings

                    spacing: Kirigami.Units.smallSpacing

                    // Not relevant to the offline upgrade use case
                    visible: !appInfo.isOfflineUpgrade

                    onImplicitWidthChanged: visibleChildrenChanged()

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
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Kirigami.UrlButton {
                                        Layout.fillWidth: true
                                        enabled: url !== ""
                                        text: modelData.name
                                        url: modelData.url
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignTop
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 3
                                        elide: Text.ElideRight
                                        color: !modelData.hasFreedom ? Kirigami.Theme.neutralTextColor: enabled ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor
                                    }

                                    // Button to open "What's the risk of proprietary software?" sheet
                                    ToolButton {
                                        visible: !modelData.hasFreedom
                                        icon.name: "help-contextual"
                                        onClicked: properietarySoftwareRiskExplanationDialog.open();

                                        ToolTip {
                                            text: i18n("What does this mean?")
                                        }
                                    }
                                }
                            }

                            // "See More licenses" link, in case there are a lot of them
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
                    // Content Rating
                    ColumnLayout {
                        width: metadataLayout.itemWidth
                        visible: appInfo.application.contentRatingText.length > 0
                        spacing: Kirigami.Units.smallSpacing

                        Label {
                            Layout.fillWidth: true
                            opacity: 0.7
                            text: i18n("Content Rating")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: appInfo.application.contentRatingMinimumAge === 0 ? "" : i18n("Age: %1+", appInfo.application.contentRatingMinimumAge)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: appInfo.application.contentRatingText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight

                            readonly property var colors: [ Kirigami.Theme.textColor, Kirigami.Theme.neutralTextColor ]
                            color: colors[appInfo.application.contentRatingIntensity]
                        }

                        Kirigami.LinkButton {
                            Layout.fillWidth: true
                            visible: appInfo.application.contentRatingDescription.length > 0
                            text: i18nc("@action", "See details…")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            elide: Text.ElideRight
                            onClicked: contentRatingDialog.open();
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
            Layout.bottomMargin: appInfo.internalSpacings * 2

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
                // Not relevant to the offline upgrade use case because we
                // display the info in the header instead
                visible: !appInfo.isOfflineUpgrade
                text: appInfo.application.comment
                type: Kirigami.Heading.Type.Primary
                level: 2
                wrapMode: Text.Wrap
                maximumLineCount: 5
                elide: Text.ElideRight
            }

            // Long app description
            Kirigami.SelectableLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: appInfo.application.longDescription
                textFormat: TextEdit.RichText
                onLinkActivated: Qt.openUrlExternally(link);
            }

            // External resources
            GridLayout {
                id: externalResourcesLayout
                readonly property int visibleButtons: (helpButton.visible ? 1 : 0)
                                                    + (homepageButton.visible ? 1: 0)
                                                    + (addonsButton.visible ? 1 : 0)
                                                    + (shareButton.visible ? 1 : 0)
                readonly property int buttonWidth: Math.round(textualContentLayout.width / columns)
                readonly property int tallestButtonHeight: Math.max(helpButton.implicitHeight,
                                                                    homepageButton.implicitHeight,
                                                                    shareButton.implicitHeight,
                                                                    addonsButton.implicitHeight)
                readonly property int minWidth: Math.max(helpButton.visible ? helpButton.implicitMinWidth : 0,
                                                                  homepageButton.visible ? homepageButton.implicitMinWidth: 0,
                                                                  addonsButton.visible ? addonsButton.implicitMinWidth : 0,
                                                                  shareButton.visible ? shareButton.implicitMinWidth : 0)
                readonly property bool stackedlayout: minWidth > Math.round(textualContentLayout.width / visibleButtons) -
                                                        (columnSpacing * (visibleButtons + 1))

                Layout.fillWidth: true
                Layout.bottomMargin: appInfo.internalSpacings * 2


                rows: stackedlayout ? visibleButtons : 1
                columns: stackedlayout ? 1: visibleButtons
                rowSpacing: Kirigami.Units.smallSpacing
                columnSpacing: Kirigami.Units.smallSpacing

                visible: visibleButtons > 0

                ApplicationResourceButton {
                    id: helpButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: externalResourcesLayout.buttonWidth
                    Layout.minimumHeight: externalResourcesLayout.tallestButtonHeight

                    visible: application.helpURL != ""

                    buttonIcon: "documentation"
                    title: i18n("Documentation")
                    subtitle: i18n("Read the project's official documentation")
                    tooltipText: application.helpURL

                    onClicked: Qt.openUrlExternally(application.helpURL);
                }

                ApplicationResourceButton {
                    id: homepageButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: externalResourcesLayout.buttonWidth
                    Layout.minimumHeight: externalResourcesLayout.tallestButtonHeight

                    visible: application.homepage != ""

                    buttonIcon: "internet-services"
                    title: i18n("Website")
                    subtitle: i18n("Visit the project's website")
                    tooltipText: application.homepage

                    onClicked: Qt.openUrlExternally(application.homepage);
                }

                ApplicationResourceButton {
                    id: addonsButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: externalResourcesLayout.buttonWidth
                    Layout.minimumHeight: externalResourcesLayout.tallestButtonHeight

                    visible: addonsView.containsAddons

                    buttonIcon: "extension-symbolic"
                    title: i18n("Addons")
                    subtitle: i18n("Install or remove additional functionality")

                    onClicked: {
                        if (addonsView.addonsCount === 0) {
                            Navigation.openExtends(application.appstreamId, appInfo.application.name)
                        } else {
                            addonsView.sheetOpen = true
                        }
                    }
                }

                ApplicationResourceButton {
                    id: shareButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: externalResourcesLayout.buttonWidth
                    Layout.minimumHeight: externalResourcesLayout.tallestButtonHeight

                    buttonIcon: "document-share"
                    title: i18nc("Exports the application's URL to an external service", "Share")
                    subtitle: i18n("Send a link for this application")
                    tooltipText: application.url.toString()
                    visible: tooltipText.length > 0 && !appInfo.isOfflineUpgrade

                    Kirigami.PromptDialog {
                        id: shareSheet
                        parent: applicationWindow().overlay
                        title: shareButton.title
                        standardButtons: Dialog.NoButton

                        Purpose.AlternativesView {
                            id: alts
                            implicitWidth: Kirigami.Units.gridUnit
                            pluginType: "ShareUrl"
                            inputData: {
                                "urls": [ application.url.toString() ],
                                "title": i18nc("The subject line for an email. %1 is the name of an application", "Check out the %1 app!", application.name)
                            }
                            onFinished: {
                                shareSheet.close()
                                if (error !== 0) {
                                    console.error("job finished with error", error, message)
                                }
                                alts.reset()
                            }
                        }
                    }

                    onClicked: {
                        shareSheet.open();
                    }
                }
            }

            Kirigami.Heading {
                visible: changelogLabel.visible
                text: i18n("What's New")
                level: 2
                type: Kirigami.Heading.Type.Primary
                wrapMode: Text.Wrap
            }

            // Changelog text
            Label {
                id: changelogLabel

                Layout.fillWidth: true
                Layout.bottomMargin: appInfo.internalSpacings * 2

                // Some backends are known to produce empty line break as a text
                visible: text !== "" && text !== "<br />"
                wrapMode: Text.WordWrap

                Component.onCompleted: appInfo.application.fetchChangelog()
                Connections {
                    target: appInfo.application
                    function onChangelogFetched(changelog) {
                        changelogLabel.text = changelog
                    }
                }
            }

            Kirigami.LoadingPlaceholder {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: Kirigami.Units.gridUnit * 15
                Layout.bottomMargin: appInfo.internalSpacings * 2
                visible: reviewsModel.fetching && !reviewsError.visible
                text: i18n("Loading reviews for %1", appInfo.application.name)
            }

            Kirigami.Heading {
                Layout.fillWidth: true
                visible: rep.count > 0 || reviewsError.visible
                text: i18n("Reviews")
                level: 2
                type: Kirigami.Heading.Type.Primary
                wrapMode: Text.Wrap
            }

            Kirigami.InlineMessage {
                id: reviewsError
                type: Kirigami.MessageType.Warning
                Layout.fillWidth: true
                visible: reviewsModel.backend && text.length > 0
                text: reviewsModel.backend ? reviewsModel.backend.errorMessage : ""
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
                Layout.bottomMargin: appInfo.internalSpacings *2

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
                    visible: appbutton.isStateAvailable && reviewsModel.backend && !reviewsError.visible && reviewsModel.backend.isResourceSupported(appInfo.application)
                    enabled: appInfo.application.isInstalled

                    text: appInfo.application.isInstalled ? i18n("Write a Review") : i18n("Install to Write a Review")
                    icon.name: "document-edit"

                    onClicked: {
                        reviewsSheet.openReviewDialog()
                    }
                }
            }

            // "Get Involved" section
            Kirigami.Heading {
                visible: getInvolvedLayout.visible
                text: i18n("Get Involved")
                level: 2
                type: Kirigami.Heading.Type.Primary
                wrapMode: Text.Wrap
            }

            GridLayout {
                id: getInvolvedLayout

                readonly property int visibleButtons: (donateButton.visible ? 1 : 0)
                                                    + (bugButton.visible ? 1 : 0)
                                                    + (contributeButton.visible ? 1 : 0)
                readonly property int buttonWidth: Math.round(textualContentLayout.width / columns)
                readonly property int tallestButtonHeight: Math.max(donateButton.implicitHeight,
                                                                    bugButton.implicitHeight,
                                                                    contributeButton.implicitHeight)
                readonly property int minWidth: Math.max(donateButton.visible ? donateButton.implicitMinWidth : 0,
                                                          bugButton.visible ? bugButton.implicitMinWidth: 0,
                                                          contributeButton.visible ? contributeButton.implicitMinWidth : 0)
                readonly property bool stackedlayout: minWidth > Math.round(textualContentLayout.width / visibleButtons) -
                                                        (columnSpacing * (visibleButtons + 1))

                Layout.fillWidth: true
                Layout.bottomMargin: appInfo.internalSpacings * 2

                rows: stackedlayout ? visibleButtons : 1
                columns: stackedlayout ? 1: visibleButtons
                rowSpacing: Kirigami.Units.smallSpacing
                columnSpacing: Kirigami.Units.smallSpacing

                visible: visibleButtons > 0

                ApplicationResourceButton {
                    id: donateButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: getInvolvedLayout.buttonWidth
                    Layout.minimumHeight: getInvolvedLayout.tallestButtonHeight

                    visible: application.donationURL != ""

                    buttonIcon: "help-donate"
                    title: i18n("Donate")
                    subtitle: i18n("Support and thank the developers by donating to their project")
                    tooltipText: application.donationURL

                    onClicked: Qt.openUrlExternally(application.donationURL);
                }

                ApplicationResourceButton {
                    id: bugButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: getInvolvedLayout.buttonWidth
                    Layout.minimumHeight: getInvolvedLayout.tallestButtonHeight

                    visible: application.bugURL != ""

                    buttonIcon: "tools-report-bug"
                    title: i18n("Report Bug")
                    subtitle: i18n("Log an issue you found to help get it fixed")
                    tooltipText: application.bugURL

                    onClicked: Qt.openUrlExternally(application.bugURL);
                }

                ApplicationResourceButton {
                    id: contributeButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: getInvolvedLayout.buttonWidth
                    Layout.minimumHeight: getInvolvedLayout.tallestButtonHeight

                    visible: application.contributeURL != ""
                    title: i18n("Contribute")
                    subtitle: i18n("Help the developers by coding, designing, testing, or translating")
                    tooltipText: application.contributeURL
                    buttonIcon: "project-development"
                    onClicked: Qt.openUrlExternally(application.contributeURL);
                }
            }

            Repeater {
                model: application.objects
                delegate: Loader {
                    property QtObject resource: appInfo.application
                    source: modelData
                    Layout.fillWidth: true
                }
            }
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
    }

    Kirigami.Dialog {
        id: allLicensesSheet
        title: i18n("All Licenses")
        standardButtons: Kirigami.Dialog.NoButton
        preferredWidth: Kirigami.Units.gridUnit * 16
        maximumHeight: Kirigami.Units.gridUnit * 20

        ColumnLayout {
            spacing: 0
            
            Repeater {
                id: listview

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
                        color: !modelData.hasFreedom ? Kirigami.Theme.neutralTextColor: enabled ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor
                    }
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: contentRatingDialog
        title: i18n("Content Rating")
        standardButtons: Kirigami.Dialog.NoButton

        Label {
            text: appInfo.application.contentRatingDescription
            textFormat: Text.MarkdownText
        }
    }

    Kirigami.PromptDialog {
        id: properietarySoftwareRiskExplanationDialog
        preferredWidth: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        title: i18n("Risks of proprietary software")

        TextEdit {
            readonly property string proprietarySoftwareExplanationPage: "https://www.gnu.org/proprietary"

            text: homepageButton.visible ?
                xi18nc("@info", "This application's source code is partially or entirely closed to public inspection and improvement. That means third parties and users like you cannot verify its operation, security, and trustworthiness, or modify and redistribute it without the authors' express permission.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways—such as harvesting your personal information, tracking your location, or transmitting the contents of your files to someone else. There is no easy way to be sure, so you should only install this application if you fully trust its authors (<link url='%1'>%2</link>).<nl/><nl/>You can learn more at <link url='%3'>%3</link>.", application.homepage, author.text, proprietarySoftwareExplanationPage) :
                xi18nc("@info", "This application's source code is partially or entirely closed to public inspection and improvement. That means third parties and users like you cannot verify its operation, security, and trustworthiness, or modify and redistribute it without the authors' express permission.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways—such as harvesting your personal information, tracking your location, or transmitting the contents of your files to someone else. There is no easy way to be sure, so you should only install this application if you fully trust its authors (%1).<nl/><nl/>You can learn more at <link url='%2'>%2</link>.", author.text, proprietarySoftwareExplanationPage)
            wrapMode: Text.Wrap
            textFormat: TextEdit.RichText
            readOnly: true

            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor

            onLinkActivated: (url) => Qt.openUrlExternally(url)

            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}
