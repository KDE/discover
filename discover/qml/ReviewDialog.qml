import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

Dialog
{
    id: reviewDialog

    property QtObject application
    property alias rating: ratingInput.rating
    property alias summary: summaryInput.text
    property alias review: reviewInput.text
    title: i18n("Reviewing '%1'", application.name)
    modality: Qt.WindowModal
    width: 500

    ColumnLayout {
        width: parent.width

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
        }

        TextArea {
            id: reviewInput
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    standardButtons: StandardButton.Ok | StandardButton.Close
}
