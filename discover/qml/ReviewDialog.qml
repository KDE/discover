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

    signal accepted()

    ColumnLayout {
        Kirigami.Heading { level: 3; text: i18n("Reviewing '%1'", application.name) }
        Label { text: i18n("Rating:") }
        Rating {
            id: ratingInput
            editable: true
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
            property bool acceptableInput: false
            Layout.fillWidth: true
            Layout.fillHeight: true

            onLengthChanged: {
                acceptableInput = length >= 15 && length < 3000
            }
        }

        Button {
            id: acceptButton
            Layout.alignment: Qt.AlignRight
            enabled: summaryInput.acceptableInput && reviewInput.acceptableInput
            text: i18n("Accept")
            onClicked: {
                reviewDialog.accepted()
                reviewDialog.sheetOpen = false
            }
        }
    }
}
