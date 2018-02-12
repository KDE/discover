import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.OverlaySheet
{
    id: reviewDialog

    property QtObject application
    readonly property alias rating: ratingInput.rating
    readonly property alias summary: summaryInput.text
    readonly property alias review: reviewInput.text
    property QtObject backend: null

    signal accepted()

    ColumnLayout {
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
        Label { text: i18n("Summary:") }
        TextField {
            id: summaryInput
            Layout.fillWidth: true
            placeholderText: i18n("Short summary...")
            validator: RegExpValidator { regExp: /.{3,70}/ }
        }

        TextArea {
            id: reviewInput
            readonly property bool acceptableInput: inputIssue.length === 0
            readonly property string inputIssue: length < 15 ? i18n("Comment too short") :
                                                 length > 3000 ? i18n("Comment too long") : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            id: acceptButton
            Layout.alignment: Qt.AlignRight
            enabled: summaryInput.acceptableInput && reviewInput.acceptableInput
            text: summaryInput.acceptableInput && reviewInput.acceptableInput ? i18n("Submit")
                : !summaryInput.acceptableInput ? i18n("Improve summary") : reviewInput.inputIssue
            onClicked: {
                reviewDialog.accepted()
                reviewDialog.sheetOpen = false
            }
        }
    }
}
