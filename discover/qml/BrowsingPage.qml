/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Felipe Kinoshita <kinofhek@gmail.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtGraphicalEffects 1.12
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.19 as Kirigami

DiscoverPage {
    id: page
    title: i18n("Featured")
    objectName: "featured"

    actions.main: window.wideScreen ? searchAction : null

    readonly property bool isHome: true

    function searchFor(text) {
        if (text.length === 0)
            return;
        Navigation.openCategory(null, "")
    }

    FeaturedModel {
        id: featuredModel
    }

    component CategoryDelegate: ColumnLayout {
        id: control

        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.topMargin: Kirigami.Units.gridUnit * 2
            Layout.leftMargin: Kirigami.Units.gridUnit * 2

            Kirigami.Heading {
                text: "Category Name"//model.categoryName

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                source: "arrow-right"
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            TapHandler {
                onTapped: console.info("GO TO CATEGORY")
            }
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.leftMargin: Kirigami.Units.gridUnit * 2
            Layout.rightMargin: Kirigami.Units.gridUnit * 2

            columns: window.wideScreen ? 3 : 1
            rows: window.wideScreen ? 3 : 9

            rowSpacing: Kirigami.Units.largeSpacing
            columnSpacing: Kirigami.Units.largeSpacing

            Repeater {
                model: DelegateModel {
                    model: featuredModel
                    rootIndex: modelIndex(index)
                    delegate: ApplicationDelegate {
                        application: model.applicationObject
                        compact: true
                    }
                }
            }
        }
    }

    signal clearSearch()

    ListView {
        id: featuredCategory
        anchors.fill: parent

        model: featuredModel
        delegate: CategoryDelegate {
            width: featuredCategory.width > 900 ? 900 : featuredCategory.width
            anchors.horizontalCenter: parent.horizontalCenter
        }

        //Kirigami.LoadingPlaceholder {
            //visible: featuredCategory.count === 0 && featuredCategory.model.isFetching
            //anchors.centerIn: parent
        //}

        Loader {
            active: featuredCategory.count === 0 && !featuredCategory.model.isFetching
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            sourceComponent: Kirigami.PlaceholderMessage {
                readonly property var helpfulError: appsRep.model.currentApplicationBackend.explainDysfunction()
                icon.name: helpfulError.iconName
                text: i18n("Unable to load applications")
                explanation: helpfulError.errorMessage

                Repeater {
                    model: helpfulError.actions
                    delegate: QQC2.Button {
                        Layout.alignment: Qt.AlignHCenter
                        action: ConvertDiscoverAction {
                            action: modelData
                        }
                    }
                }
            }
        }
    }

    footer: ColumnLayout {
        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
            visible: Kirigami.Settings.isMobile && inlineMessage.visible
        }

        Kirigami.InlineMessage {
            id: inlineMessage
            icon.name: updateAction.icon.name
            showCloseButton: true
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing * 2
            text: i18n("Updates are available")
            visible: Kirigami.Settings.isMobile && ResourcesModel.updatesCount > 0
            actions: Kirigami.Action {
                icon.name: "go-next"
                text: i18nc("Short for 'show updates'", "Show")
                onTriggered: updateAction.trigger()
            }
        }
    }
}
