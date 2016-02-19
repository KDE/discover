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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 1.0
import org.kde.discover.app 1.0

Item {
    id: appInfo
    property QtObject application: null
    clip: true

    readonly property var icon: application.icon
    readonly property string title: application.name

    ConditionalLoader {
        anchors.fill: parent
        condition: app.isCompact

        componentFalse: Item {
            readonly property real proposedMargin: (width-app.actualWidth)/2

            GridLayout {
                x: proposedMargin
                width: app.actualWidth
                height: parent.height
                columns: 2
                rows: 2

                PageHeader {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    topMargin: 0

                    RowLayout {
                        QIconItem {
                            Layout.preferredHeight: col.height
                            Layout.preferredWidth: col.height

                            icon: appInfo.application.icon
                        }

                        ColumnLayout {
                            id: col
                            Layout.fillWidth: true

                            spacing: 0
                            Heading {
                                text: appInfo.application.name
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                text: appInfo.application.comment
                                wrapMode: Text.WordWrap
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }
                        InstallApplicationButton {
                            id: installButton
                            application: appInfo.application
                            additionalItem:  Rating {
                                readonly property QtObject ratingInstance: application.rating
                                visible: ratingInstance!=null
                                rating:  ratingInstance==null ? 0 : ratingInstance.rating
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: app.actualWidth/3

                    Item {
                        id: screenshotsPlaceholder
                        Layout.fillWidth: true
                        Layout.preferredHeight: width/1.618

                        ApplicationScreenshots {
                            initialParent: screenshotsPlaceholder
                            fullParent: appInfo
                            application: appInfo.application
                        }
                    }
                    ApplicationDetails {
                        Layout.fillWidth: true
                        application: appInfo.application
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                }
                ScrollView {
                    id: scroll
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: app.actualWidth/2

                    ApplicationDescription {
                        width: scroll.viewport.width-margin/2
                        application: appInfo.application
                        isInstalling: installButton.isActive
                    }
                }
            }
        }
        componentTrue: ScrollView {
            id: scroll
            flickableItem.flickableDirection: Flickable.VerticalFlick


            ColumnLayout {
                width: scroll.viewport.width-desc.margin
                x: desc.margin/2

                GridItem {
                    Layout.fillWidth: true
                    height: conts.Layout.minimumHeight + 2*internalMargin

                    ColumnLayout {
                        id: conts
                        anchors.fill: parent

                        RowLayout {
                            Layout.fillWidth: true
                            QIconItem {
                                Layout.preferredHeight: title.height
                                Layout.preferredWidth: title.height

                                icon: appInfo.application.icon
                                Layout.alignment: Qt.AlignTop
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Heading {
                                    id: title
                                    text: appInfo.application.name
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: appInfo.application.comment
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                                Rating {
                                    readonly property QtObject ratingInstance: application.rating
                                    visible: ratingInstance!=null
                                    rating:  ratingInstance==null ? 0 : ratingInstance.rating
                                    starSize: title.paintedHeight

                                    Text { text: i18n(" (%1)", parent.ratingInstance.ratingCount) }
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                Layout.fillWidth: true
                                text: i18n("Update")
                                onClicked: ResourcesModel.installApplication(appInfo.application)
                                opacity: appInfo.application.canUpgrade ? 1 : 0

                                TransactionListener {
                                    id: listener
                                    resource: appInfo.application
                                }
                                enabled: !listener.isActive
                            }
                            Button {
                                Layout.fillWidth: true
                                text: i18n("Launch")
                            }
                        }
                    }
                }

                Item {
                    id: screenshotsPlaceholder
                    Layout.fillWidth: true
                    Layout.minimumHeight: details.height

                    ApplicationScreenshots {
                        application: appInfo.application
                        initialParent: screenshotsPlaceholder
                        fullParent: scroll
                    }
                }

                ApplicationDescription {
                    id: desc
                    Layout.fillWidth: true

                    application: appInfo.application
                    z: -1
                }

                ApplicationDetails {
                    id: details
                    Layout.fillWidth: true
                    application: appInfo.application
                }
            }
        }
    }
}
