// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

BigscreenUI.ApplicationWindow {
    id: root

    width: 1280
    height: 720
    visible: true
    title: i18n("Discover")

    readonly property string updatesText: Discover.ResourcesModel.updatesCount > 0
        ? i18nc("@title %1 is the number of updates", "Updates (%1)", Discover.ResourcesModel.updatesCount)
        : i18nc("@title", "Updates")

    component Section: Kirigami.Action {
        property Item page
    }

    readonly property list<Section> sections: [
        Section {
            text: i18nc("@title", "Home")
            icon.name: "go-home-symbolic"
            page: HomePage {}
        },
        Section {
            text: i18nc("@title", "Browse")
            icon.name: "view-list-icons"
        },
        Section {
            text: i18nc("@title", "Library")
            icon.name: "view-list-details"
        },
        Section {
            text: root.updatesText
            icon.name: "update-none"
        },
        Section {
            text: i18nc("@title", "Search")
            icon.name: "search"
            page: SearchPage {}
        }
    ]

    property int currentIndex: 0
    readonly property Item currentPage: sections[currentIndex].page

    function showSection(index: int): void {
        if (sections[index].page) {
            currentIndex = index;
            showPage(currentPage);
        }
        sectionSidebar.currentIndex = currentIndex;
    }

    function openApplication(application: Discover.AbstractResource): void {
        if (application) {
            pageStack.push(appPage, { application: application });
        }
    }

    function openSettings(): void {
        if (!(pageStack.currentItem instanceof SettingsPage)) {
            pageStack.push(settingsPage);
        }
    }

    function showMessage(message: string): void {
        toast.show(message, "dialog-information");
    }

    initialPage: currentPage

    sidebar: BigscreenUI.Sidebar {
        id: sectionSidebar

        title: i18n("Discover")
        iconName: "plasmadiscover"

        actions: [...root.sections, separatorAction, settingsAction]
        onActivated: index => {
            if (index < root.sections.length) {
                root.showSection(index);
            } else {
                currentIndex = root.currentIndex;
            }
        }

        Kirigami.Action {
            id: separatorAction
            separator: true
        }
        Kirigami.Action {
            id: settingsAction
            text: i18nc("@title", "Settings")
            icon.name: "configure"
            onTriggered: root.openSettings()
        }
    }

    BigscreenUI.BackHandler.enabled: pageStack.depth <= 1
    BigscreenUI.BackHandler.onActivated: {
        if (!sectionSidebar.activeFocus) {
            sectionSidebar.forceActiveFocus();
        } else if (currentIndex !== 0) {
            showSection(0);
        }
    }

    // Installs keep going in the background when the window is closed, so quit once done
    onClosing: {
        if (!Discover.TransactionModel.hasActiveTransactions) {
            Qt.quit();
        }
    }

    Connections {
        target: Discover.TransactionModel
        enabled: !root.visible
        function onActiveTransactionsChanged(): void {
            if (!Discover.TransactionModel.hasActiveTransactions) {
                Qt.quit();
            }
        }
    }

    Connections {
        target: Discover.ResourcesModel
        function onPassiveMessage(message: string): void {
            root.showMessage(message);
        }
    }

    BigscreenUI.Toast {
        id: toast
        z: 1
    }

    Component {
        id: appPage
        AppPage {}
    }

    Component {
        id: settingsPage
        SettingsPage {}
    }
}
