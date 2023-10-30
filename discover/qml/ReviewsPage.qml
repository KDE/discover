/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

Kirigami.OverlaySheet {
    id: page

    property alias model: sortModel.sourceModel
    property alias sortModel: sortModel
    property alias sortRole: sortModel.sortRoleName

    readonly property QtObject reviewsBackend: resource.backend.reviewsBackend
    readonly property var resource: model.resource

    readonly property var rd: ReviewDialog {
        id: reviewDialog

        application: page.resource
        backend: page.reviewsBackend
        onAccepted: backend.submitReview(resource, summary, review, rating, name)
    }

    function openReviewDialog() {
        page.visible = false
        reviewDialog.open()
    }

    implicitWidth: Math.min(Kirigami.Units.gridUnit * 70, Math.max(Kirigami.Units.gridUnit * 30, parent.width * 0.8))
    parent: applicationWindow().overlay

    header: GridLayout {
        id: headerLayout
        width: parent.width
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: columnSpacing
        rows: 2
        columns: 3

        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.columnSpan: 3
            wrapMode: Text.WordWrap
            text: i18n("Reviews for %1", page.resource.name)
        }

        Button {
            id: reviewButton

            visible: page.reviewsBackend !== null
            enabled: page.resource.isInstalled
            text: i18n("Write a Review…")
            onClicked: page.openReviewDialog()
        }
        // This layout is intended as a mere container of messageLabel, don't put anything else in it
        RowLayout {
            id: inlineMessageParent
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Label {
                id: messageLabel
                Layout.fillWidth: true
                parent: inlineMessageParent.width >= messageLabelMetrics.width ? inlineMessageParent : newlineMessageParent
                text: i18n("Install this app to write a review")
                wrapMode: Text.WordWrap
                visible: !reviewButton.enabled
                opacity: 0.6
                TextMetrics {
                    id: messageLabelMetrics
                    font: messageLabel.font
                    text: messageLabel.text
                }
            }
        }

        ActionGroup {
            id: sortGroup
            exclusive: true
        }
        Kirigami.ActionToolBar {
            id: actionToolBar
            Layout.minimumWidth: implicitWidth + 1
            // This is to align it under the close button, some api for it could be good
            Layout.rightMargin: showCloseButton ? -Kirigami.Units.iconSizes.smallMedium : 0

            alignment: Qt.AlignRight
            Layout.fillWidth: false
            actions: Kirigami.Action {
                text: i18n("Sort: %1", sortGroup.checkedAction.text)
                icon.name: "view-sort-symbolic"
                displayHint: Kirigami.DisplayHint.KeepVisible
                Kirigami.Action {
                    text: i18nc("@label:listbox Most relevant reviews", "Most Relevant")
                    ActionGroup.group: sortGroup
                    checkable: true
                    checked: sortModel.sortRoleName === "wilsonScore"
                    onTriggered: sortModel.sortRoleName = "wilsonScore"
                }
                Kirigami.Action {
                    text: i18nc("@label:listbox Most recent reviews", "Most Recent")
                    ActionGroup.group: sortGroup
                    checkable: true
                    checked: sortModel.sortRoleName === "date"
                    onTriggered: sortModel.sortRoleName = "date"
                }
                Kirigami.Action {
                    text: i18nc("@label:listbox Reviews with the highest ratings", "Highest Ratings")
                    ActionGroup.group: sortGroup
                    checkable: true
                    checked: sortModel.sortRoleName === "rating"
                    onTriggered: sortModel.sortRoleName = "rating"
                }
            }
        }
        // This layout is intended as a mere container of messageLabel, don't put anything else in it
        RowLayout {
            id: newlineMessageParent
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Layout.columnSpan: 3
        }
    }

    ListView {
        id: reviewsView

        model: KItemModels.KSortFilterProxyModel {
            id: sortModel
            filterRoleName: "shouldShow"
            filterRowCallback: (sourceRow, sourceParent) => {
                return sourceModel.data(sourceModel.index(sourceRow, 0, sourceParent), filterRole) === true;
            }
            // need to do it afterwads as direct binding won't work, because at startup sortRoleName will be empty
            onSortRoleNameChanged: sortOrder = Qt.DescendingOrder
        }
        clip: true
        topMargin: Kirigami.Units.largeSpacing
        leftMargin: Kirigami.Units.largeSpacing
        rightMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing
        implicitWidth: Math.max(Kirigami.Units.gridUnit * 25, Math.round(applicationWindow().overlay.width / 2))
        // Still preload some items to make the scrollbar behave better, but can't preload all the comments as some apps like Firefox have thousands of them which will freeze Discover for minutes
        cacheBuffer: height * 2
        reuseItems: true
        contentWidth: width - leftMargin - rightMargin

        delegate: ReviewDelegate {
            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
            separator: index !== ListView.view.count - 1
            onMarkUseful: page.model.markUseful(index, useful)
        }
    }
}
