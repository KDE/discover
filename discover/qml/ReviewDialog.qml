import QtQuick 2.3
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.OverlaySheet
{
    id: reviewDialog

    property QtObject application
    readonly property alias rating: ratingInput.rating
    readonly property alias summary: titleInput.text
    readonly property alias review: reviewInput.text
    property QtObject backend: null

    signal accepted()

    ColumnLayout {
        height: Kirigami.Units.gridUnit * 22
        Kirigami.Heading { level: 3; text: i18n("Reviewing '%1'", application.name) }
        Label { text: i18n("Rating:") }
        Rating {
            id: ratingInput
            editable: true
        }

        Label {
            visible: reviewDialog.backend.userName.length > 0
            text: visible ? i18n("Submission name: %1", reviewDialog.backend.userName) : ""
        }
        Label { text: i18n("Title:") }
        TextField {
            id: titleInput
            Layout.fillWidth: true
            validator: RegExpValidator { regExp: /.{3,70}/ }
        }
        Label { text: i18n("Review:") }
        TextArea {
            id: reviewInput
            readonly property bool acceptableInput: length > 15 && length < 3000
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            id: acceptButton
            Layout.alignment: Qt.AlignRight
            enabled: rating > 2 && titleInput.acceptableInput && reviewInput.acceptableInput
            text: {
                if (rating < 2) return i18n("Enter a rating");
                if (! titleInput.acceptableInput) return i18n("Write a title");
                if (reviewInput.length < 15) return i18n("Keep writing...");
                if (reviewInput.length > 3000) return i18n("Too long!");
                return i18n("Submit review");
            }
            onClicked: {
                reviewDialog.accepted()
                reviewDialog.sheetOpen = false
            }
        }
    }
}
