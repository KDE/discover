import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.OverlaySheet
{
    id: reviewDialog
    parent: applicationWindow().overlay

    property QtObject application
    readonly property alias rating: ratingInput.rating
    readonly property alias name: nameInput.text
    readonly property alias summary: titleInput.text
    readonly property alias review: reviewInput.text
    property QtObject backend: null

    signal accepted()

    title: i18n("Reviewing %1", application.name)

    contentItem: ColumnLayout {
        Layout.maximumWidth: Kirigami.Units.gridUnit * 24
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Rating {
                id: ratingInput
                Kirigami.FormData.label: i18n("Rating:")
                editable: true
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
        }

        RowLayout {
            QQC2.Label {
                id: instructionalLabel
                Layout.fillWidth: true
                text: {
                    if (rating < 2) return i18n("Enter a rating");
                    if (! titleInput.acceptableInput) return i18n("Write the title");
                    if (reviewInput.length === 0) return i18n("Write the review");
                    if (reviewInput.length < 15) return i18n("Keep writing…");
                    if (reviewInput.length > 3000) return i18n("Too long!");
                    if (nameInput.length < 1) return i18nc("@info:usagetip", "Insert a name");
                    return "";
                }
                wrapMode: Text.WordWrap
                opacity: 0.6
                visible: text.length > 0
            }
            Item {
                Layout.fillWidth: true
                visible: !instructionalLabel.visible
            }
            QQC2.Button {
                id: acceptButton
                enabled: !instructionalLabel.visible
                text: i18n("Submit review")
                icon.name: "document-send"
                onClicked: {
                    reviewDialog.accepted()
                    reviewDialog.sheetOpen = false
                }
            }
        }
    }
}
