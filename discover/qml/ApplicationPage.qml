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
import Qt5Compat.GraphicalEffects
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
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

    Kirigami.Theme.colorSet: Kirigami.Theme.Window

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
            required property int index
            required property var model

            QQC2.ActionGroup.group: sourcesGroup
            text: model.availableVersion
                ? i18n("%1 - %2", model.displayOrigin, model.availableVersion)
                : model.displayOrigin
            icon.name: model.sourceIcon
            checkable: true
            checked: appInfo.application === model.application
            onTriggered: if (index >= 0) {
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
    bottomPadding: Kirigami.Units.largeSpacing
    verticalPadding: undefined
    horizontalPadding: undefined

    header: ColumnLayout {
        id: topObjectsLayout

        spacing: 0

        Repeater {
            id: topObjectsRepeater

            model: application.topObjects

            delegate: Loader {
                required property string modelData

                property QtObject resource: appInfo.application

                Layout.fillWidth: true
                Layout.preferredHeight: item ? item.height : 0
                source: modelData
            }
        }
    }

    // Scrollable page content
    ColumnLayout {
        id: pageLayout

        spacing: 0

        // Colored header with app icon, name
        QQC2.Control {
            Layout.fillWidth: true

            bottomInset: -Kirigami.Units.gridUnit * 4
            topPadding: Kirigami.Units.largeSpacing * 2

            background: Item {
                Item {
                    anchors.fill: parent

                    Rectangle {
                        anchors.fill: parent
                        color:  appImageColorExtractor.dominant
                        opacity: 0.2
                    }

                    Kirigami.Icon {
                        visible: source
                        scale: 1.8
                        anchors.fill: parent

                        source: appInfo.application.icon

                        implicitWidth: 512
                        implicitHeight: 512
                    }

                    layer.enabled: true
                    layer.effect: HueSaturation {
                        cached: true

                        saturation: 1.9

                        layer {
                            enabled: true
                            effect: FastBlur {
                                cached: true
                                radius: 100
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: -1.0; color: "transparent" }
                        GradientStop { position: 1.0; color: Kirigami.Theme.backgroundColor }
                    }
                }
            }

            contentItem: RowLayout {
                // App icon, name, author, and rating
                RowLayout {
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    ColumnLayout {
                        spacing: 0

                        Layout.leftMargin: Kirigami.Units.largeSpacing
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                        Layout.fillWidth: true

                        RowLayout {
                            // App name
                            Kirigami.Heading {
                                text: appInfo.application.name
                                type: Kirigami.Heading.Type.Primary
                                wrapMode: Text.Wrap
                                maximumLineCount: 5
                                elide: Text.ElideRight
                            }

                            // Rating
                            RowLayout {
                                opacity: 0.6
                                Layout.fillWidth: true

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
                    }

                    // App icon
                    Kirigami.Icon {
                        implicitWidth: Kirigami.Units.iconSizes.huge
                        implicitHeight: Kirigami.Units.iconSizes.huge
                        source: appInfo.application.icon
                        Layout.alignment:  Qt.AlignTop
                    }
                }
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

        // Short description
        FormCard.FormHeader {
            // Not relevant to the offline upgrade use case because we
            // display the info in the header instead
            visible: !appInfo.isOfflineUpgrade
            title: appInfo.application.comment
            Accessible.role: Accessible.Heading
        }

        // Long app description
        FormCard.FormCard {
            Kirigami.SelectableLabel {
                objectName: "applicationDescription" // for appium tests
                leftPadding: Kirigami.Units.gridUnit
                rightPadding: Kirigami.Units.gridUnit
                topPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                wrapMode: Text.WordWrap
                text: appInfo.application.longDescription
                textFormat: TextEdit.RichText
                onLinkActivated: link => Qt.openUrlExternally(link);

                Layout.fillWidth: true
            }
        }

        // External resources
        FormCard.FormGridContainer {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing

            readonly property int visibleButtons: (helpButton.visible ? 1 : 0)
                                                + (homepageButton.visible ? 1 : 0)
                                                + (addonsButton.visible ? 1 : 0)
                                                + (shareButton.visible ? 1 : 0)

            visible: visibleButtons > 0

            infoCards: [
                FormCard.FormGridContainer.InfoCard {
                    id: helpButton

                    visible: application.helpURL.toString() !== ""

                    buttonIcon: "documentation"
                    title: i18n("Documentation")
                    subtitle: i18n("Read the project's official documentation")
                    tooltipText: application.helpURL.toString()

                    action: Kirigami.Action {
                        onTriggered: Qt.openUrlExternally(application.helpURL);
                    }
                },
                FormCard.FormGridContainer.InfoCard {
                    id: homepageButton

                    visible: application.homepage.toString() !== ""

                    buttonIcon: "internet-services"
                    title: i18n("Website")
                    subtitle: i18n("Visit the project's website")
                    tooltipText: application.homepage.toString()

                    action: Kirigami.Action {
                        onTriggered: Qt.openUrlExternally(application.homepage);
                    }
                },

                FormCard.FormGridContainer.InfoCard {
                    id: addonsButton

                    visible: addonsView.containsAddons

                    buttonIcon: "extension-symbolic"
                    title: i18n("Addons")
                    subtitle: i18n("Install or remove additional functionality")

                    action: Kirigami.Action {
                        onTriggered: {
                            if (addonsView.addonsCount === 0) {
                                Navigation.openExtends(application.appstreamId, appInfo.application.name)
                            } else {
                                addonsView.visible = true
                            }
                        }
                    }
                },

                FormCard.FormGridContainer.InfoCard {
                    id: shareButton

                    buttonIcon: "document-share"
                    title: i18nc("Exports the application's URL to an external service", "Share")
                    subtitle: i18n("Send a link for this application")
                    tooltipText: application.url.toString()
                    visible: tooltipText.length > 0 && !appInfo.isOfflineUpgrade

                    property Kirigami.PromptDialog shareSheet: Kirigami.PromptDialog {
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

                    action: Kirigami.Action {
                        onTriggered: shareSheet.open();
                    }
                }
            ]
        }

        // Metadata
        FormCard.FormHeader {
            title: i18nc("@title:group", "Metadata")
            visible: !appInfo.isOfflineUpgrade
        }

        FormCard.FormGridContainer {
            Layout.fillWidth: true

            // Not relevant to offline updates
            visible: !appInfo.isOfflineUpgrade

            infoCards: [
                // Version
                FormCard.FormGridContainer.InfoCard {
                    title: i18nc("@label", "Version:")
                    subtitle: appInfo.application.versionString
                },
                // Size
                FormCard.FormGridContainer.InfoCard {
                    title: i18nc("@label", "Size:")
                    subtitle: appInfo.application.sizeDescription
                },
                // Licenses
                FormCard.FormGridContainer.InfoCard {
                    readonly property bool isProprietary: appInfo.application.licenses.some((license) => !license.hasFreedom)
                    readonly property Kirigami.Action showWarning: Kirigami.Action {
                        onTriggered: properietarySoftwareRiskExplanationDialog.open();
                    }

                    buttonIcon: isProprietary ? "help-contextual" : undefined

                    title: i18np("License:", "Licenses:", appInfo.application.licenses.length)
                    subtitle: if (appInfo.application.licenses.length === 0) {
                        return i18nc("The app does not provide any licenses", "Unknown");
                    } else {
                        let content = appInfo.application.licenses
                            .slice(0, 2)
                            .map((license) => {
                                if (!license.url) {
                                    return license.hasFreedom ? license.name : `<a style="color: ${Kirigami.Theme.neutralTextColor}" href="discover:proprietary">${license.name}</a>`;
                                } else {
                                    return `<a style="color: ${!license.hasFreedom ? Kirigami.Theme.neutralTextColor: Kirigami.Theme.linkColor}" href="${license.url}">${license.name}</a>`;
                                }
                            })
                            .join(i18nc("list separator", ", "))

                        // "See More licenses" link, in case there are a lot of them
                        if (application.licenses.length > 3) {
                            content += i18nc("End of sentense", ". ");
                            content += `<a href="discover:seemore">${i18np("See more…", "See more…", appInfo.application.licenses.length)}</a>`;
                        }

                        return content;
                    }
                    subtitleTextFormat: Text.RichText

                    action: isProprietary ? showWarning : 0

                    function linkActivated(link: string): void {
                        if (link == "discover:seemore") {
                            allLicensesSheet.open();
                            return;
                        }
                        if (link == "discover:proprietary") {
                            properietarySoftwareRiskExplanationDialog.open();
                            return;
                        }
                        Qt.openUrlExternally(link);
                    }
                },
                // Content Rating
                FormCard.FormGridContainer.InfoCard {
                    readonly property var colors: [ Kirigami.Theme.textColor, Kirigami.Theme.neutralTextColor ]

                    title: i18nc("@label", "Content Rating:")
                    subtitle: {
                        let content = "";
                        if (appInfo.application.contentRatingMinimumAge !== 0) {
                            content += i18n("Age: %1+", appInfo.application.contentRatingMinimumAge) + "<br />";
                        }
                        content += appInfo.application.contentRatingText;
                        return content;
                    }
                    visible: subtitle.length > 0

                    action: appInfo.application.contentRatingDescription.length > 0 ? seeDetails : null

                    // color: colors[appInfo.application.contentRatingIntensity]

                    readonly property Kirigami.Action seeDetails: Kirigami.Action {
                        onTriggered: contentRatingDialog.open();
                    }
                }
            ]
        }

        FormCard.FormHeader {
            visible: changelogLabelCard.visible
            title: i18n("What's New")
        }

        // Changelog text
        FormCard.FormCard {
            id: changelogLabelCard

            // Some backends are known to produce empty line break as a text
            visible: changelogLabel.text !== "" && changelogLabel.text !== "<br />"

            FormCard.FormTextDelegate {
                id: changelogLabel

                Component.onCompleted: appInfo.application.fetchChangelog()
                Connections {
                    target: appInfo.application
                    function onChangelogFetched(changelog) {
                        changelogLabel.text = changelog
                    }
                }
            }
        }

        FormCard.FormHeader {
            title: i18n("Reviews")
            visible: reviewsSheet.sortModel.count > 0 && !reviewsLoadingPlaceholder.visible && !reviewsError.visible
        }

        FormCard.FormCard {
            visible: reviewsSheet.sortModel.count > 0 && !reviewsLoadingPlaceholder.visible && !reviewsError.visible

            Kirigami.LoadingPlaceholder {
                id: reviewsLoadingPlaceholder
                Layout.fillWidth: true
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
            FormCard.AbstractFormDelegate {
                background: null
                contentItem: Flow {
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
            }
        }

        // "Get Involved" section
        FormCard.FormHeader {
            visible: getInvolvedLayout.visible
            title: i18n("Get Involved")
        }

        FormCard.FormGridContainer {
            id: getInvolvedLayout

            Layout.fillWidth: true

            readonly property int visibleButtons: (donateButton.visible ? 1 : 0)
                                                + (bugButton.visible ? 1 : 0)
                                                + (contributeButton.visible ? 1 : 0)

            visible: visibleButtons > 0

            infoCards: [
                FormCard.FormGridContainer.InfoCard {
                    id: donateButton

                    visible: application.donationURL.toString() !== ""

                    buttonIcon: "help-donate"
                    title: i18n("Donate")
                    subtitle: i18n("Support and thank the developers by donating to their project")
                    tooltipText: application.donationURL.toString()

                    action: Kirigami.Action {
                        onTriggered: Qt.openUrlExternally(application.donationURL);
                    }
                },

                FormCard.FormGridContainer.InfoCard {
                    id: bugButton

                    visible: application.bugURL.toString() !== ""

                    buttonIcon: "tools-report-bug"
                    title: i18n("Report Bug")
                    subtitle: i18n("Log an issue you found to help get it fixed")
                    tooltipText: application.bugURL.toString()

                    action: Kirigami.Action {
                        onTriggered: Qt.openUrlExternally(application.bugURL);
                    }
                },

                FormCard.FormGridContainer.InfoCard {
                    id: contributeButton

                    visible: application.contributeURL.toString() !== ""

                    buttonIcon: "project-development"
                    title: i18n("Contribute")
                    subtitle: i18n("Help the developers by coding, designing, testing, or translating")
                    tooltipText: application.contributeURL.toString()

                    action: Kirigami.Action {
                        onTriggered: Qt.openUrlExternally(application.contributeURL);
                    }
                }
            ]
        }

        Repeater {
            model: application.bottomObjects
            delegate: Loader {
                required property int index
                required property string modelData

                // Context property for loaded component
                readonly property Discover.AbstractResource resource: appInfo.application

                source: modelData
                Layout.fillWidth: true
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
