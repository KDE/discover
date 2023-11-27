import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

Kirigami.PromptDialog {
    id: reviewDialog

    required property Discover.AbstractResource application
    required property Discover.AbstractReviewsBackend backend

    readonly property alias rating: ratingInput.value
    readonly property alias name: nameInput.text
    readonly property alias summary: titleInput.text
    readonly property alias review: reviewInput.text

    preferredWidth: Kirigami.Units.gridUnit * 30

    title: i18n("Reviewing %1", application.name)

    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            id: submitAction
            text: i18n("Submit review")
            icon.name: "document-send"
            enabled: !instructionalLabel.visible
            onTriggered: reviewDialog.accept();
        }
    ]

    ColumnLayout {
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Rating {
                id: ratingInput
                Kirigami.FormData.label: i18n("Rating:")
                readOnly: false
            }
            QQC2.TextField {
                id: nameInput
                Kirigami.FormData.label: i18n("Name:")
                visible: page.reviewsBackend !== null && reviewDialog.backend.preferredUserName.length > 0
                Layout.fillWidth: true
                readOnly: !reviewDialog.backend.supportsNameChange
                text: visible ? reviewDialog.backend.preferredUserName : ""
            }
            QQC2.TextField {
                id: titleInput
                Kirigami.FormData.label: i18n("Title:")
                Layout.fillWidth: true
                validator: RegularExpressionValidator { regularExpression: /.{3,70}/ }
            }
        }

        QQC2.TextArea {
            id: reviewInput
            readonly property bool acceptableInput: length > 15 && length < 3000
            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 8
            KeyNavigation.priority: KeyNavigation.BeforeItem
            wrapMode: TextEdit.Wrap
        }

        QQC2.Label {
            id: instructionalLabel
            Layout.fillWidth: true
            text: {
                if (rating < 2) {
                    return i18n("Enter a rating");
                }
                if (!titleInput.acceptableInput) {
                    return i18n("Write the title");
                }
                if (reviewInput.length === 0) {
                    return i18n("Write the review");
                }
                if (reviewInput.length < 15) {
                    return i18n("Keep writing…");
                }
                if (reviewInput.length > 3000) {
                    return i18n("Too long!");
                }
                if (nameInput.visible && nameInput.length < 1) {
                    return i18nc("@info:usagetip", "Insert a name");
                }
                return "";
            }
            wrapMode: Text.WordWrap
            opacity: 0.6
            visible: text.length > 0
        }
    }

    onOpened: {
        ratingInput.forceActiveFocus(Qt.PopupFocusReason);
    }

    Component.onCompleted: {
        const submitButton = customFooterButton(submitAction);
        if (submitButton) {
            reviewInput.KeyNavigation.tab = submitButton;
            submitButton.KeyNavigation.tab = ratingInput;
        }
    }
}
