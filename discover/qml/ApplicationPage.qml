/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.purpose as Purpose

DiscoverPage {
    id: appInfo

    title: "" // It would duplicate the text in the header right below it
    clip: true

    required property Discover.AbstractResource application

    readonly property int visibleReviews: 3
    readonly property int internalSpacings: Kirigami.Units.largeSpacing
    readonly property int pageContentMargins: Kirigami.Units.gridUnit
    readonly property bool availableFromOnlySingleSource: !originsMenuAction.visible

    // Usually this page is not the top level page, but when we are, isHome being
    // true will ensure that the search field suggests we are searching in the list
    // of available apps, not inside the app page itself. This will happen when
    // Discover is launched e.g. from krunner or otherwise requested to show a
    // specific application on launch.
    readonly property bool isHome: true

    readonly property bool isOfflineUpgrade: application.packageName === "discover-offline-upgrade"

    ReviewsPage {
        id: reviewsSheet
        parent: appInfo.QQC2.Overlay.overlay
        model: Discover.ReviewsModel {
            id: reviewsModel
            resource: appInfo.application
            preferredSortRole: reviewsSheet.sortRole
        }
        Component.onCompleted: reviewsSheet.sortRole = reviewsModel.preferredSortRole
    }

    actions: [
        appbutton.isActive ? appbutton.cancelAction : appbutton.action,
        invokeAction,
        originsMenuAction
    ]

    QQC2.ActionGroup {
        id: sourcesGroup
        exclusive: true
    }

    // Multi-source origin display and switcher
    Kirigami.Action {
        id: originsMenuAction

        text: i18nc("@item:inlistbox %1 is the name of an app source e.g. \"Flathub\" or \"Ubuntu\"", "From %1", appInfo.application.displayOrigin)
        visible: children.length > 1
        children: sourcesGroup.actions
    }

    Instantiator {
        // alternativeResourcesModel
        model: Discover.ResourcesProxyModel {
            allBackends: true
            resourcesUrl: appInfo.application.url
        }
        delegate: QQC2.Action {
            required property var model

            QQC2.ActionGroup.group: sourcesGroup
            text: model.availableVersion
                ? i18n("%1 - %2", model.displayOrigin, model.availableVersion)
                : model.displayOrigin
            icon.name: model.sourceIcon
            checkable: true
            checked: appInfo.application === model.application
            onTriggered: {
                appInfo.application = model.application
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

    InstallApplicationButton {
        id: appbutton
        Layout.rightMargin: Kirigami.Units.smallSpacing
        application: appInfo.application
        visible: false
        availableFromOnlySingleSource: appInfo.availableFromOnlySingleSource
    }

    Kirigami.ImageColors {
        id: appImageColorExtractor
        source: appInfo.application.icon
    }

    padding: 0
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    // Scrollable page content
    ColumnLayout {
        id: pageLayout

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: appInfo.internalSpacings

        // Colored header with app icon, name, and metadata
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: headerLayout.implicitHeight + (headerLayout.anchors.topMargin * 2)
            color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, appImageColorExtractor.dominant, 0.1)

            GridLayout {
                id: headerLayout

                readonly property bool stackedMode: appBasicInfoLayout.implicitWidth + columnSpacing + appMetadataLayout.implicitWidth > (pageLayout.width - anchors.leftMargin - anchors.rightMargin)

                columns: stackedMode ? 1 : 2
                rows: stackedMode ? 2 : 1
                columnSpacing: appInfo.internalSpacings
                rowSpacing: appInfo.internalSpacings

                anchors {
                    top: parent.top
                    topMargin: appInfo.internalSpacings
                    left: parent.left
                    leftMargin: appInfo.internalSpacings
                    right: parent.right
                    rightMargin: appInfo.internalSpacings
                }


                // App icon, name, author, and rating
                RowLayout {
                    id: appBasicInfoLayout
                    Layout.maximumWidth: headerLayout.implicitWidth
                    Layout.alignment: headerLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignLeft
                    spacing: appInfo.internalSpacings

                    // App icon
                    Kirigami.Icon {
                        implicitWidth: Kirigami.Units.iconSizes.huge
                        implicitHeight: Kirigami.Units.iconSizes.huge
                        source: appInfo.application.icon
                    }

                    // App name, author, and rating
                    ColumnLayout {

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
                        QQC2.Label {
                            id: author

                            Layout.fillWidth: true
                            visible: text.length > 0

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

                            // Not relevant to the offline upgrade use case
                            visible: !appInfo.isOfflineUpgrade

                            Rating {
                                value: appInfo.application.rating ? appInfo.application.rating.sortableRating : 0
                                starSize: author.font.pointSize
                                precision: Rating.Precision.HalfStar
                            }

                            QQC2.Label {
                                text: appInfo.application.rating ? i18np("%1 rating", "%1 ratings", appInfo.application.rating.ratingCount) : i18n("No ratings yet")
                            }
                        }
                    }
                }

                // Metadata
                // Not using Kirigami.FormLayout here because we never want it to move into Mobile
                // mode and we also want to customize the spacing, neither of which it lets us do
                GridLayout {
                    id: appMetadataLayout

                    Layout.alignment: headerLayout.stackedMode ? Qt.AlignHCenter : Qt.AlignRight

                    columns: 2
                    rows: Math.ceil(appMetadataLayout.visibleChildren.count / 2)
                    columnSpacing: Kirigami.Units.smallSpacing
                    rowSpacing: 0

                    // Not relevant to offline updates
                    visible: !appInfo.isOfflineUpgrade

                    // Version
                    QQC2.Label {
                        text: i18n("Version:")
                        Layout.alignment: Qt.AlignRight
                    }
                    QQC2.Label {
                        text: appInfo.application.versionString
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    // Size
                    QQC2.Label {
                        text: i18n("Size:")
                        Layout.alignment: Qt.AlignRight
                    }
                    QQC2.Label {
                        text: appInfo.application.sizeDescription
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    // Licenses
                    QQC2.Label {
                        text: i18np("License:", "Licenses:", appInfo.application.licenses.length)
                        Layout.alignment: Qt.AlignRight
                    }
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            visible : appInfo.application.licenses.length === 0
                            text: i18nc("The app does not provide any licenses", "Unknown")
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                        }

                        Repeater {
                            visible: appInfo.application.licenses.length > 0
                            model: appInfo.application.licenses.slice(0, 2)
                            delegate: RowLayout {
                                id: delegate

                                required property var modelData

                                spacing: 0

                                Kirigami.UrlButton {
                                    enabled: url !== ""
                                    text: delegate.modelData.name
                                    url: delegate.modelData.url
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignTop
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    color: !delegate.modelData.hasFreedom ? Kirigami.Theme.neutralTextColor : (enabled ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor)
                                }

                                // Button to open "What's the risk of proprietary software?" sheet
                                QQC2.ToolButton {
                                    visible: !delegate.modelData.hasFreedom
                                    icon.name: "help-contextual"
                                    onClicked: properietarySoftwareRiskExplanationDialog.open();

                                    QQC2.ToolTip {
                                        text: i18n("What does this mean?")
                                    }
                                }
                            }
                        }

                        // "See More licenses" link, in case there are a lot of them
                        Kirigami.LinkButton {
                            visible: application.licenses.length > 3
                            text: i18np("See more…", "See more…", appInfo.application.licenses.length)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            elide: Text.ElideRight
                            onClicked: allLicensesSheet.open();
                        }
                    }

                    // Content Rating
                    QQC2.Label {
                        visible: appInfo.application.contentRatingText.length > 0
                        text: i18n("Content Rating:")
                        Layout.alignment: Qt.AlignRight
                    }
                    RowLayout {
                        visible: appInfo.application.contentRatingText.length > 0
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            visible: text.length > 0
                            text: appInfo.application.contentRatingMinimumAge === 0 ? "" : i18n("Age: %1+", appInfo.application.contentRatingMinimumAge)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }

                        QQC2.Label {
                            text: appInfo.application.contentRatingText
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight

                            readonly property var colors: [ Kirigami.Theme.textColor, Kirigami.Theme.neutralTextColor ]
                            color: colors[appInfo.application.contentRatingIntensity]
                        }

                        Kirigami.LinkButton {
                            visible: appInfo.application.contentRatingDescription.length > 0
                            text: i18nc("@action", "See details…")
                            elide: Text.ElideRight
                            onClicked: contentRatingDialog.open();
                        }
                    }
                }
            }

            Kirigami.Separator {
                width: parent.width
                anchors.top: parent.bottom
            }
        }

        // Screenshots
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.leftMargin: appInfo.pageContentMargins
            Layout.rightMargin: appInfo.pageContentMargins

            visible: carousel.hasFailed
            icon.name: "image-missing"
            text: i18nc("@info placeholder message", "Screenshots not available for %1", appInfo.application.name)
        }

        CarouselInlineView {
            id: carousel

            Layout.fillWidth: true
            // This roughly replicates scaling formula for the screenshots
            // gallery on FlatHub website, adjusted to scale with gridUnit
            Layout.minimumHeight: Math.round((16 + 1/9) * Kirigami.Units.gridUnit)
            Layout.maximumHeight: 30 * Kirigami.Units.gridUnit
            Layout.preferredHeight: Math.round(width / 2) + Math.round((2 + 7/9) * Kirigami.Units.gridUnit)
            Layout.topMargin: appInfo.internalSpacings

            edgeMargin: appInfo.internalSpacings
            visible: carouselModel.count > 0 && !hasFailed

            carouselModel: Discover.ScreenshotsModel {
                application: appInfo.application
            }
        }

        ColumnLayout {
            id: topObjectsLayout

            // InlineMessage components are supposed to manage their spacing
            // internally. However, at least for now they require some
            // assistance from outside to stack them one after another.
            spacing: 0

            Layout.fillWidth: true

            // Cancel out parent layout's spacing, making this component effectively zero-sized when empty.
            // When non-empty, the very first top margin is provided by this layout, but bottom margins
            // are implemented by Loaders that have visible loaded items.
            Layout.topMargin: hasVisibleObjects ? 0 : -pageLayout.spacing
            Layout.bottomMargin: -pageLayout.spacing

            property bool hasVisibleObjects: false

            function bindVisibility() {
                hasVisibleObjects = Qt.binding(() => {
                    for (let i = 0; i < topObjectsRepeater.count; i++) {
                        const loader = topObjectsRepeater.itemAt(i);
                        const item = loader.item;
                        if (item?.visible) {
                            return true;
                        }
                    }
                    return false;
                });
            }

            Timer {
                id: bindVisibilityTimer

                running: false
                repeat: false
                interval: 0

                onTriggered: topObjectsLayout.bindVisibility()
            }

            Repeater {
                id: topObjectsRepeater

                model: appInfo.application.topObjects

                delegate: Loader {
                    required property string modelData

                    Layout.fillWidth: item?.Layout.fillWidth ?? false
                    Layout.topMargin: 0
                    Layout.leftMargin: appInfo.pageContentMargins
                    Layout.rightMargin: appInfo.pageContentMargins
                    Layout.bottomMargin: item?.visible ? appInfo.internalSpacings : 0
                    Layout.preferredHeight: item?.visible ? item.implicitHeight : 0

                    onModelDataChanged: {
                        setSource(modelData, { resource: Qt.binding(() => appInfo.application) });
                    }
                }
                onItemAdded: (index, item) => {
                    bindVisibilityTimer.start();
                }
                onItemRemoved: (index, item) => {
                    bindVisibilityTimer.start();
                }
            }
        }

        // Layout for textual content; this isn't in the main ColumnLayout
        // because we want it to be bounded to a maximum width
        ColumnLayout {
            id: textualContentLayout

            Layout.fillWidth: true
            Layout.margins: appInfo.pageContentMargins
            Layout.alignment: Qt.AlignHCenter

            spacing: appInfo.internalSpacings

            // Short description
            // Not using Kirigami.Heading here because that component doesn't
            // support selectable text, and we want this to be selectable because
            // it's also used to show the path for local packages, and that makes
            // sense to be selectable
            Kirigami.SelectableLabel {
                Layout.fillWidth: true
                // Not relevant to the offline upgrade use case because we
                // display the info in the header instead
                visible: !appInfo.isOfflineUpgrade
                text: appInfo.application.comment
                wrapMode: Text.Wrap

                // Match `level: 1` in Kirigami.Heading
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                font.weight: Font.DemiBold

                Accessible.role: Accessible.Heading
            }

            // Long app description
            Kirigami.SelectableLabel {
                objectName: "applicationDescription" // for appium tests
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: appInfo.application.longDescription
                textFormat: TextEdit.RichText
                onLinkActivated: link => Qt.openUrlExternally(link);
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

                    visible: application.helpURL.toString() !== ""

                    buttonIcon: "documentation"
                    title: i18n("Documentation")
                    subtitle: i18n("Read the project's official documentation")
                    tooltipText: application.helpURL.toString()

                    onClicked: Qt.openUrlExternally(application.helpURL);
                }

                ApplicationResourceButton {
                    id: homepageButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: externalResourcesLayout.buttonWidth
                    Layout.minimumHeight: externalResourcesLayout.tallestButtonHeight

                    visible: application.homepage.toString() !== ""

                    buttonIcon: "internet-services"
                    title: i18n("Website")
                    subtitle: i18n("Visit the project's website")
                    tooltipText: application.homepage.toString()

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
                            addonsView.visible = true
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
                        parent: appInfo.QQC2.Overlay.overlay
                        title: shareButton.title
                        standardButtons: QQC2.Dialog.NoButton

                        Purpose.AlternativesView {
                            id: alts
                            implicitWidth: Kirigami.Units.gridUnit
                            pluginType: "ShareUrl"
                            inputData: {
                                "urls": [ application.url.toString() ],
                                "title": i18nc("The subject line for an email. %1 is the name of an application", "Check out the %1 app!", application.name)
                            }
                            onFinished: (/*var*/ output, /*int*/ error, /*string*/ message) => {
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
            QQC2.Label {
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


            Kirigami.Heading {
                Layout.fillWidth: true
                visible: reviewsSheet.sortModel.count > 0 && !reviewsLoadingPlaceholder.visible && !reviewsError.visible
                text: i18n("Reviews")
                level: 2
                type: Kirigami.Heading.Type.Primary
                wrapMode: Text.Wrap
            }

            Kirigami.LoadingPlaceholder {
                id: reviewsLoadingPlaceholder
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: Kirigami.Units.gridUnit * 15
                Layout.bottomMargin: appInfo.internalSpacings * 2
                visible: reviewsModel.fetching
                text: i18n("Loading reviews for %1", appInfo.application.name)
            }

            Kirigami.PlaceholderMessage {
                id: reviewsError
                Layout.fillWidth: true
                visible: reviewsModel.backend && reviewsModel.backend.errorMessage.length > 0 && text.length > 0 && reviewsModel.count === 0 && !reviewsLoadingPlaceholder.visible
                icon.name: "text-unflow"
                text: i18nc("@info placeholder message", "Reviews for %1 are temporarily unavailable", appInfo.application.name)
                explanation: reviewsModel.backend ? reviewsModel.backend.errorMessage : ""
            }

            ReviewsStats {
                visible: reviewsModel.count > 3
                Layout.fillWidth: true
                application: appInfo.application
                reviewsModel: reviewsModel
                sortModel: reviewsSheet.sortModel
                visibleReviews: appInfo.visibleReviews
                compact: appInfo.compact
            }

            // Review-related buttons
            Flow {
                Layout.fillWidth: true
                Layout.bottomMargin: appInfo.internalSpacings * 2

                spacing: appInfo.internalSpacings

                QQC2.Button {
                    visible: reviewsModel.count > visibleReviews

                    text: i18nc("@action:button", "Show All Reviews")
                    icon.name: "view-visible"

                    onClicked: {
                        reviewsSheet.open()
                    }
                }

                QQC2.Button {
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

                    visible: application.donationURL.toString() !== ""

                    buttonIcon: "help-donate"
                    title: i18n("Donate")
                    subtitle: i18n("Support and thank the developers by donating to their project")
                    tooltipText: application.donationURL.toString()

                    onClicked: Qt.openUrlExternally(application.donationURL);
                }

                ApplicationResourceButton {
                    id: bugButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: getInvolvedLayout.buttonWidth
                    Layout.minimumHeight: getInvolvedLayout.tallestButtonHeight

                    visible: application.bugURL.toString() !== ""

                    buttonIcon: "tools-report-bug"
                    title: i18n("Report Bug")
                    subtitle: i18n("Log an issue you found to help get it fixed")
                    tooltipText: application.bugURL.toString()

                    onClicked: Qt.openUrlExternally(application.bugURL);
                }

                ApplicationResourceButton {
                    id: contributeButton

                    Layout.fillWidth: true
                    Layout.maximumWidth: getInvolvedLayout.buttonWidth
                    Layout.minimumHeight: getInvolvedLayout.tallestButtonHeight

                    visible: application.contributeURL.toString() !== ""

                    buttonIcon: "project-development"
                    title: i18n("Contribute")
                    subtitle: i18n("Help the developers by coding, designing, testing, or translating")
                    tooltipText: application.contributeURL.toString()

                    onClicked: Qt.openUrlExternally(application.contributeURL);
                }
            }

            Repeater {
                model: appInfo.application.bottomObjects

                delegate: Loader {
                    required property string modelData

                    Layout.fillWidth: true

                    onModelDataChanged: {
                        setSource(modelData, { resource: Qt.binding(() => appInfo.application) });
                    }
                }
            }
        }
    }

    AddonsView {
        id: addonsView

        application: appInfo.application
        parent: appInfo.QQC2.Overlay.overlay
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
                model: appInfo.application.licenses

                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property var modelData

                    contentItem: Kirigami.UrlButton {
                        enabled: url !== ""
                        text: delegate.modelData.name
                        url: delegate.modelData.url
                        horizontalAlignment: Text.AlignLeft
                        color: !delegate.modelData.hasFreedom
                            ? Kirigami.Theme.neutralTextColor
                            : (enabled ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor)
                    }
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: contentRatingDialog
        parent: appInfo.QQC2.Overlay.overlay
        title: i18n("Content Rating")
        preferredWidth: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        QQC2.Label {
            text: appInfo.application.contentRatingDescription
            textFormat: Text.MarkdownText
            wrapMode: Text.Wrap
        }
    }

    Kirigami.PromptDialog {
        id: properietarySoftwareRiskExplanationDialog
        parent: appInfo.QQC2.Overlay.overlay
        preferredWidth: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        title: i18n("Risks of proprietary software")

        TextEdit {
            readonly property string proprietarySoftwareExplanationPage: "https://www.gnu.org/proprietary"

            text: homepageButton.visible
                ? xi18nc("@info", "This application's source code is partially or entirely closed to public inspection and improvement. That means third parties and users like you cannot verify its operation, security, and trustworthiness, or modify and redistribute it without the authors' express permission.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways—such as harvesting your personal information, tracking your location, or transmitting the contents of your files to someone else. There is no easy way to be sure, so you should only install this application if you fully trust its authors (<link url='%1'>%2</link>).<nl/><nl/>You can learn more at <link url='%3'>%3</link>.", application.homepage, author.text, proprietarySoftwareExplanationPage)
                : xi18nc("@info", "This application's source code is partially or entirely closed to public inspection and improvement. That means third parties and users like you cannot verify its operation, security, and trustworthiness, or modify and redistribute it without the authors' express permission.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways—such as harvesting your personal information, tracking your location, or transmitting the contents of your files to someone else. There is no easy way to be sure, so you should only install this application if you fully trust its authors (%1).<nl/><nl/>You can learn more at <link url='%2'>%2</link>.", author.text, proprietarySoftwareExplanationPage)
            wrapMode: Text.Wrap
            textFormat: TextEdit.RichText
            readOnly: true

            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor

            onLinkActivated: url => Qt.openUrlExternally(url)

            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}
