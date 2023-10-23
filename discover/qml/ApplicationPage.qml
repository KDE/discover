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
    readonly property int internalSpacings: padding * 2
    readonly property bool availableFromOnlySingleSource: !originsMenuAction.visible

    // Usually this page is not the top level page, but when we are, isHome being
    // true will ensure that the search field suggests we are searching in the list
    // of available apps, not inside the app page itself. This will happen when
    // Discover is launched e.g. from krunner or otherwise requested to show a
    // specific application on launch.
    readonly property bool isHome: true

    readonly property bool isOfflineUpgrade: application.packageName === "discover-offline-upgrade"

    readonly property int smallButtonSize: Kirigami.Units.iconSizes.small + (Kirigami.Units.smallSpacing * 2)

    function colorForLicenseType(licenseType: string): string {
        switch(licenseType) {
            case "free":
                return Kirigami.Theme.positiveTextColor;
            case "non-free":
                return Kirigami.Theme.neutralTextColor;
            case "proprietary":
                return Kirigami.Theme.negativeTextColor
            case "unknown":
            default:
                return Kirigami.Theme.neutralTextColor;
        }
    }

    function explanationForLicenseType(licenseType: string): string {
        let freeSoftwareUrl = "https://www.gnu.org/philosophy/free-sw.html"
        let fsfUrl = "https://www.fsf.org/"
        let osiUrl = "https://opensource.org/"
        let proprietarySoftwareUrl = "https://www.gnu.org/proprietary"
        let hasHomepageUrl = application.homepage.toString().length > 0

        switch(licenseType) {
            case "proprietary":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "Only install %1 if you fully trust its authors because it is <emphasis strong='true'>proprietary</emphasis>: Your freedom to use, modify, and redistribute this application is restricted, and its source code is partially or entirely closed to public inspection and improvement. This means third parties and users like you cannot verify its operation, security, and trustworthiness.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways — such as harvesting your personal information, tracking your location, or transmitting the contents of your data to someone else. Only use it if you fully trust its authors. More information may be available on <link url='%2'>the application's website</link>.<nl/><nl/>Learn more at <link url='%3'>%3</link>.",
                                appInfo.application.name,
                                appInfo.application.homepage.toString(),
                                proprietarySoftwareUrl)
                } else {
                    return xi18nc("@info", "Only install %1 if you fully trust its authors because it is <emphasis strong='true'>proprietary</emphasis>: Your freedom to use, modify, and redistribute this application is restricted, and its source code is partially or entirely closed to public inspection and improvement. This means third parties and users like you cannot verify its operation, security, and trustworthiness.<nl/><nl/>The application may be perfectly safe to use, or it may be acting against you in various ways — such as harvesting your personal information, tracking your location, or transmitting the contents of your data to someone else. Only use it if you fully trust its authors. Learn more at <link url='%2'>%2</link>.",
                                  appInfo.application.name,
                                  proprietarySoftwareUrl)
                }

            case "non-free":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "%1 uses one or more licenses not certified as “Free Software” by either the <link url='%2'>Free Software Foundation</link> or the <link url='%3'>Open Source Initiative</link>. This means your freedom to use, study, modify, and share it may be restricted in some ways.<nl/><nl/>Make sure to read the license text and understand any restrictions before using the software.<nl/><nl/>If the license does not even grant access to read the source code, make sure you fully trust the authors, as no one else can verify the trustworthiness and security of its code to ensure that it is not acting against you in hidden ways. More information may be available on <link url='%4'>the application's website</link>.<nl/><nl/>Learn more at <link url='%5'>%5</link>.",
                                appInfo.application.name,
                                fsfUrl,
                                osiUrl,
                                appInfo.application.homepage.toString(),
                                freeSoftwareUrl);
                } else {
                    return xi18nc("@info", "%1 uses one or more licenses not certified as “Free Software” by either the <link url='%2'>Free Software Foundation</link> or the <link url='%3'>Open Source Initiative</link>. This means your freedom to use, study, modify, and share it may be restricted in some ways.<nl/><nl/>Make sure to read the license text and understand any restrictions before using the software.<nl/><nl/>If the license does not even grant access to read the source code, make sure you fully trust the authors, as no one else can verify the trustworthiness and security of its code to ensure that it is not acting against you in hidden ways.<nl/><nl/>Learn more at <link url='%4'>%4</link>.",
                                  appInfo.application.name,
                                  fsfUrl,
                                  osiUrl,
                                  freeSoftwareUrl);
                }

            case "unknown":
                if (hasHomepageUrl) {
                    return xi18nc("@info", "%1 does not indicate under which license it is distributed. You may be able to determine this on <link url='%2'>the application's website</link>. Find it there or contact the author if you want to use this application for anything other than private personal use.",
                                 appInfo.application.name,
                                 appInfo.application.homepage.toString());
                } else {
                    return i18nc("@info", "%1 does not indicate under which license it is distributed. Contact the application's author if you want to use it for anything other than private personal use.",
                                 appInfo.application.name);
                }

            case "free":
            default:
                return "";
        }
    }

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
        addonsAction,
        shareAction,
        appbutton.isActive ? appbutton.cancelAction : appbutton.action,
        invokeAction,
        originsMenuAction
    ]

    QQC2.ActionGroup {
        id: sourcesGroup
        exclusive: true
    }

    Kirigami.Action {
        id: shareAction
        text: i18nc("@action:button share a link to this app", "Share")
        icon.name: "document-share"
        visible: application.url.toString().length > 0 && !appInfo.isOfflineUpgrade
        onTriggered: shareSheet.open()
    }

    Kirigami.Action {
        id: addonsAction
        text: i18nc("@action:button", "Add-ons")
        icon.name: "extension-symbolic"
        visible: addonsView.containsAddons
        onTriggered: {
            if (addonsView.addonsCount === 0) {
                Navigation.openExtends(application.appstreamId, appInfo.application.name)
            } else {
                addonsView.visible = true
            }
        }
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
        icon.name: "media-playback-start-symbolic"
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

    Kirigami.PromptDialog {
        id: shareSheet
        parent: applicationWindow().overlay
        implicitWidth: Kirigami.Units.gridUnit * 20
        title: i18nc("@title:window", "Share Link to Application")
        standardButtons: QQC2.Dialog.NoButton

        Purpose.AlternativesView {
            id: alts
            Layout.fillWidth: true
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

        // Header with app icon, name, author, and rating
        RowLayout {
            Layout.topMargin: Kirigami.Units.largeSpacing * 2
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            // App icon
            Kirigami.Icon {
                implicitWidth: Kirigami.Units.iconSizes.huge
                implicitHeight: Kirigami.Units.iconSizes.huge
                source: appInfo.application.icon
                Layout.alignment:  Qt.AlignTop
            }

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
        }

        // Screenshots
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true

            visible: carousel.hasFailed
            icon.name: "image-missing"
            text: i18nc("@info placeholder message", "Screenshots not available for %1", appInfo.application.name)
        }

        CarouselInlineView {
            id: carousel

            Layout.fillWidth: true
            // Undo page paddings so that the header touches the edges. We don't
            // want it to actually be in the header: area since then it wouldn't
            // scroll away, which we do want.
            Layout.leftMargin: -appInfo.leftPadding
            Layout.rightMargin: -appInfo.rightPadding
            // This roughly replicates scaling formula for the screenshots
            // gallery on FlatHub website, adjusted to scale with gridUnit
            Layout.minimumHeight: Math.round((16 + 1/9) * Kirigami.Units.gridUnit)
            Layout.maximumHeight: 30 * Kirigami.Units.gridUnit
            Layout.preferredHeight: Math.round(width / 2) + Math.round((2 + 7/9) * Kirigami.Units.gridUnit)

            edgeMargin: appInfo.padding
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
            visible: appInfo.application.longDescription.length > 0

            Kirigami.SelectableLabel {
                objectName: "applicationDescription" // for appium tests
                leftPadding: Kirigami.Units.gridUnit
                rightPadding: Kirigami.Units.gridUnit
                topPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                wrapMode: Text.WordWrap
                readonly property int headerFontSize: Math.round(Kirigami.Theme.defaultFont.pixelSize * 1.15) // similar to Kirigami.Heading level 2
                text: `<style>h3 { font-size: ${headerFontSize}; font-weight: 600; }</style> ${appInfo.application.longDescription}`
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

                    action: shareAction
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
                    visible: subtitle.length > 0
                },
                // Size
                FormCard.FormGridContainer.InfoCard {
                    title: i18nc("@label", "Size:")
                    subtitle: appInfo.application.sizeDescription
                },
                // Licenses
                FormCard.FormGridContainer.InfoCard {
                    title: i18np("License:", "Licenses:", appInfo.application.licenses.length)
                    subtitle: if (appInfo.application.licenses.length === 0) {
                        return i18nc("The app does not provide any licenses", "<a href=\"discover:unknown\">Unknown</a>");
                    } else {
                        let content = appInfo.application.licenses
                            .slice(0, 2)
                            .map((license) => {
                                if (!license.url) {
                                    if (license.licenseType === "unknown" || license.licenseType === "non-free" || license.licenseType === "proprietary") {
                                        return `<a style="color: ${Kirigami.Theme.neutralTextColor}" href="discover:${license.licenseType}">${license.name}</a>`;
                                    }
                                    return license.name;
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

                    function linkActivated(link: string): void {
                        if (link == "discover:seemore") {
                            allLicensesSheet.open();
                            return;
                        }
                        if (link.startsWith('discover:')) {
                            const parts = string.split(':');
                            if (parts.length <= 1) {
                                console.error("Incorrect link", link)
                                return;
                            }
                            licenseDetailsDialog.openWithLicenseType(parts[1])
                            return;
                        }
                        Qt.openUrlExternally(link);
                    }
                },
                // Content Rating
                FormCard.FormGridContainer.InfoCard {
                    title: i18nc("@label The app is suitable for people of the following ages or older", "Ages:")
                    subtitle: application.contentRatingMinimumAge === 0
                                ? i18nc("@item As in, the app is suitable for everyone", "Everyone")
                                : i18nc("@item %1 is a person's age in number of years",
                                        "%1+", appInfo.application.contentRatingMinimumAge)

                    action: appInfo.application.contentRatingDescription.length > 0 ? seeDetails : null

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

            FormCard.AbstractFormDelegate {
                id: changelogLabel

                Layout.fillWidth: true
                Layout.preferredHeight: contentItem.paintedHeight + topPadding + bottomPadding

                background: null

                contentItem: QQC2.Label {
                    readonly property int headerFontSize: Math.round(Kirigami.Theme.defaultFont.pixelSize * 1.25) // similar to Kirigami.Heading level 3
                    textFormat: Text.RichText
                    text: `<style>h3 { font-size: ${headerFontSize}; font-weight: 400; }</style> ${changelogLabel.text}`
                }

                Component.onCompleted: appInfo.application.fetchChangelog()
                Connections {
                    target: appInfo.application
                    function onChangelogFetched(changelog: string): void {
                        changelogLabel.text = changelog;
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
                model: reviewsSheet.model
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

                onModelDataChanged: {
                    setSource(modelData, { resource: Qt.binding(() => appInfo.application) });
                }

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
                    background: null
                    id: delegate

                    required property var modelData

                    contentItem: Kirigami.UrlButton {
                        enabled: url !== ""
                        text: delegate.modelData.name
                        url: delegate.modelData.url
                        horizontalAlignment: Text.AlignLeft
                        color: appInfo.colorForLicenseType(delegate.modelData.licenseType)
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

    Kirigami.Dialog {
        id: licenseDetailsDialog

        function openWithLicenseType(licenseType: string): void {
            licenseExplanation.text = appInfo.explanationForLicenseType(licenseType);
            open();
        }

        parent: appInfo.QQC2.Overlay.overlay
        width: Kirigami.Units.gridUnit * 25
        standardButtons: Kirigami.Dialog.NoButton

        title: i18nc("@title:window", "License Information")

        TextEdit {
            id: licenseExplanation

            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing

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
