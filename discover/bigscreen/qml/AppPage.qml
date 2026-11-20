// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

BigscreenUI.Page {
    id: root

    required property Discover.AbstractResource application

    readonly property bool upgradeable: application.state === Discover.AbstractResource.Upgradeable

    readonly property list<string> badges: [
        upgradeable ? i18n("Update available") : "",
        application.isInstalled && !upgradeable ? i18n("Installed") : "",
        application.contentRatingMinimumAge > 0 ? i18nc("@info minimum age to use an app", "%1+", application.contentRatingMinimumAge) : "",
    ].filter(badge => badge.length > 0)

    readonly property list<var> details: [
        { label: i18nc("@label", "Version"), value: application.versionString },
        { label: i18nc("@label", "Size"), value: application.sizeDescription },
        { label: i18nc("@label", "Source"), value: application.displayOrigin },
        { label: i18nc("@label", "License"), value: Array.from(application.licenses).map(license => license.name).join(", ") },
        { label: i18nc("@label", "Released"), value: application.releaseDate.getTime() ? application.releaseDate.toLocaleDateString(Qt.locale(), Locale.ShortFormat) : "" },
    ].filter(info => info.value && info.value.length > 0)

    BigscreenUI.BackHandler.onActivated: BigscreenUI.ApplicationWindow.window.pageStack.pop()

    background: AppBackdrop {
        icon: root.application.icon
        // Screenshots are only added after the first or removed, which changes the count
        source: screenshots.count > 0 ? screenshots.data(screenshots.index(0, 0), Discover.ScreenshotsModel.ScreenshotUrl) : ""
    }

    Discover.ScreenshotsModel {
        id: screenshots
        application: root.application
    }

    BigscreenUI.RailView {
        id: rails

        anchors.fill: parent
        anchors.topMargin: root.contentMargins.top - topMargin
        bottomMargin: root.contentMargins.bottom
        focus: true

        FocusScope {
            implicitHeight: Math.max(tile.height, hero.implicitHeight)

            AppTile {
                id: tile

                x: root.contentMargins.left
                width: Math.min(Kirigami.Units.gridUnit * 14, Math.round(parent.width * 0.2))
                height: width
                icon: root.application.icon
                iconScale: 0.6
                shadow.size: Kirigami.Units.gridUnit * 2
                shadow.yOffset: Kirigami.Units.largeSpacing
            }

            BigscreenUI.HeroBanner {
                id: hero

                anchors.left: tile.right
                anchors.leftMargin: Kirigami.Units.gridUnit * 3
                anchors.right: parent.right
                anchors.rightMargin: root.contentMargins.right

                overline: root.application.displayOrigin
                title: root.application.name
                titlePixelSize: BigscreenUI.Units.titleFontPixelSize
                metadata: [root.application.author]
                badges: root.badges
                description: root.application.comment

                InstallButton {
                    id: installButton
                    focus: !openButton.visible
                    application: root.application
                    onRemoveRequested: removeDialog.open()
                }
                BigscreenUI.Button {
                    id: openButton
                    focus: visible
                    visible: root.application.isInstalled && root.application.canExecute && !installButton.isActive
                    text: root.application.executeLabel
                    icon.name: "media-playback-start"
                    onClicked: root.application.invokeApplication()
                }
            }
        }

        BigscreenUI.ContentRail {
            id: screenshotsRail

            title: i18nc("@title", "Screenshots")
            model: screenshots

            delegate: BigscreenUI.MediaCard {
                required property int index
                required property url small_image_url

                artworkWidth: Kirigami.Units.gridUnit * 22
                artworkHeight: Math.round(artworkWidth * 9 / 16)
                source: small_image_url
                placeholderIconName: "image-x-generic"
                Accessible.name: i18nc("@info", "Screenshot %1 of %2", index + 1, screenshotsRail.count)
                onClicked: root.BigscreenUI.ApplicationWindow.window.pageStack.push(screenshotPage, { currentIndex: index })
            }
        }

        FocusScope {
            id: aboutSection

            // RailView hides rows with no items
            readonly property int count: root.application.longDescription.length > 0 || root.details.length > 0 ? 1 : 0

            implicitHeight: aboutLayout.implicitHeight

            ColumnLayout {
                id: aboutLayout

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: aboutSection.SafeArea.margins.left
                anchors.rightMargin: aboutSection.SafeArea.margins.right
                spacing: Kirigami.Units.largeSpacing * 2

                BigscreenUI.Label {
                    id: aboutTitle
                    Layout.fillWidth: true
                    text: i18nc("@title", "About")
                    type: BigscreenUI.Label.Section
                    elide: Text.ElideRight
                    Accessible.role: Accessible.Heading
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.gridUnit * 3

                    ColumnLayout {
                        id: descriptionColumn

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        visible: descriptionLabel.text.length > 0
                        spacing: Kirigami.Units.largeSpacing * 2

                        BigscreenUI.Label {
                            id: descriptionLabel

                            Layout.fillWidth: true
                            text: root.application.longDescription
                            textFormat: Text.StyledText
                            wrapMode: Text.Wrap
                            maximumLineCount: Math.max(4, Math.floor(availableHeight / (descriptionMetrics.height * lineHeight)))
                            elide: Text.ElideRight

                            readonly property real availableHeight: rails.height - rails.topMargin - rails.bottomMargin
                                - aboutTitle.height - aboutLayout.spacing
                                - showMoreButton.implicitHeight - descriptionColumn.spacing

                            FontMetrics {
                                id: descriptionMetrics
                                font: descriptionLabel.font
                            }
                        }

                        BigscreenUI.Button {
                            id: showMoreButton
                            focus: true
                            visible: descriptionLabel.truncated
                            text: i18nc("@action:button", "Show more")
                            icon.name: "documentinfo"
                            onClicked: root.BigscreenUI.ApplicationWindow.window.pageStack.push(descriptionPage)
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                        Layout.fillWidth: !descriptionColumn.visible
                        visible: root.details.length > 0
                        spacing: Kirigami.Units.largeSpacing * 2

                        Repeater {
                            model: root.details

                            delegate: ColumnLayout {
                                id: detail

                                required property var modelData

                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                Accessible.role: Accessible.StaticText
                                Accessible.name: modelData.label + ": " + modelData.value

                                BigscreenUI.Label {
                                    Layout.fillWidth: true
                                    text: detail.modelData.label
                                    color: Kirigami.Theme.disabledTextColor
                                    type: BigscreenUI.Label.Small
                                    elide: Text.ElideRight
                                }
                                BigscreenUI.Label {
                                    Layout.fillWidth: true
                                    text: detail.modelData.value
                                    type: BigscreenUI.Label.Heading
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    BigscreenUI.Dialog {
        id: removeDialog

        title: i18nc("@title:window", "Remove %1?", root.application.name)
        standardButtons: BigscreenUI.Dialog.Cancel | BigscreenUI.Dialog.Ok

        contentItem: BigscreenUI.Label {
            text: i18n("You can install it again later.")
            wrapMode: Text.Wrap
        }

        openFocusItem: standardButton(BigscreenUI.Dialog.Cancel)
        Component.onCompleted: standardButton(BigscreenUI.Dialog.Ok).text = i18nc("@action:button", "Remove")

        onAccepted: Discover.ResourcesModel.removeApplication(root.application)
        onClosed: installButton.forceActiveFocus()
    }

    Component {
        id: descriptionPage

        DescriptionPage {
            application: root.application
        }
    }

    Component {
        id: screenshotPage

        ScreenshotPage {
            model: screenshots
        }
    }
}
