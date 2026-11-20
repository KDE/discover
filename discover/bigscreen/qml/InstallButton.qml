// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

BigscreenUI.Button {
    id: root

    required property Discover.AbstractResource application

    readonly property alias isActive: listener.isActive
    readonly property bool isLoading: application?.state === Discover.AbstractResource.Broken

    signal removeRequested()

    visible: isActive || !(application?.isInstalled ?? false) || (application?.isRemovable ?? false)
    enabled: !isLoading && (!isActive || listener.isCancellable)

    text: {
        if (isActive) {
            return listener.progress > 0
                ? i18nc("%1 is a status such as Downloading, %2 is a percentage", "%1 %2%", listener.statusText, listener.progress)
                : listener.statusText;
        }
        if (isLoading) {
            return i18n("Loading…");
        }
        if (application?.isInstalled) {
            return i18n("Remove");
        }
        return i18n("Install");
    }

    icon.name: {
        if (isActive) {
            return "dialog-cancel";
        }
        return application?.isInstalled ? "edit-delete" : "download";
    }

    onClicked: {
        if (isActive) {
            listener.cancel();
        } else if (application.isInstalled) {
            removeRequested();
        } else {
            Discover.ResourcesModel.installApplication(application);
        }
    }

    Discover.TransactionListener {
        id: listener
        resource: root.application
    }

    BigscreenUI.ProgressBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        anchors.bottomMargin: Kirigami.Units.smallSpacing
        implicitHeight: 4
        visible: root.isActive && listener.progress > 0
        value: listener.progress / 100
    }
}
