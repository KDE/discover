/*
 *   Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import org.kde.kirigami 2.5 as Kirigami

Kirigami.Page
{
    id: page
    property var aboutData: discoverAboutData
    property var aboutLibraries: discoverAboutLibraries

    contextualActions: [
        KirigamiActionBridge { action: app.action("help_report_bug") }
    ]

    title: i18n("About")
    header: ColumnLayout {
        GridLayout {
            columns: 2
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge

            Kirigami.Icon {
                Layout.rowSpan: 2
                Layout.fillHeight: true
                Layout.minimumWidth: height
                Layout.rightMargin: Kirigami.Units.largeSpacing
                source: page.aboutData.programLogo || page.aboutData.programIconName
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                text: page.aboutData.displayName + " " + page.aboutData.version
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                level: 2
                text: page.aboutData.shortDescription
            }
        }
        TabBar {
            Layout.fillWidth: true
            id: bar
            TabButton { text: i18n("About") }
            TabButton { text: i18n("Libraries") }
            TabButton { text: i18n("Authors") }
        }
    }

    Component {
        id: licencePage
        Kirigami.ScrollablePage {
            property alias text: content.text
            TextArea {
                id: content
                readOnly: true
            }
        }
    }

    SwipeView {
        anchors.fill: parent
        currentIndex: bar.currentIndex
        interactive: false
        ColumnLayout {
            Label {
                text: aboutData.shortDescription
                visible: text.length > 0
            }
            Label {
                text: aboutData.otherText
                visible: text.length > 0
            }
            Label {
                text: aboutData.copyrightStatement
                visible: text.length > 0
            }
            UrlButton {
                url: aboutData.homepage
                visible: url.length > 0
            }

            Repeater {
                id: rep
                model: aboutData.licenses
                delegate: LinkButton {
                    text: modelData.name
                    onClicked: applicationWindow().pageStack.push(licencePage, { text: modelData.text, title: modelData.name } )
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
        Label {
            id: libraries
            text: page.aboutLibraries
        }
        Kirigami.CardsListView {
            header: Label {
                readonly property string bugAddress: aboutData.bugAddress || "https://bugs.kde.org"
                readonly property string bugDisplay: aboutData.bugAddress ? ("mailto:" + aboutData.bugAddress) : "https://bugs.kde.org"
                text: i18n("Please use <a href=\"" +  + bugAddress + "\">" + bugDisplay + "</a> to report bugs.\n")
            }
            model: aboutData.authors
            delegate: Kirigami.AbstractCard {
                contentItem: RowLayout {
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                    Kirigami.Icon {
                        Layout.fillHeight: true
                        Layout.minimumWidth: Kirigami.Units.iconSizes.medium
                        Layout.maximumWidth: Kirigami.Units.iconSizes.medium
                        source: "https://www.gravatar.com/avatar/" + Qt.md5(modelData.emailAddress) + "?d=404&s=" + Kirigami.Units.iconSizes.medium
                        fallback: "user"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: i18n("%1 <%2>", modelData.name, modelData.emailAddress)
                    }
                }
            }
        }
    }
}
