import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

Dialog
{
    property QtObject application
    property alias rating: ratingInput.rating
    property alias summary: summaryInput.text
    property alias review: reviewInput.text
    id: reviewDialog
    title: i18n("Reviewing %1", application.name)
    modality: Qt.WindowModal

    height: 200
    width: 200
    
    ColumnLayout {
        width: parent.width
        height: parent.height

        Label { text: i18n("Rating:") }
        Rating {
            id: ratingInput
            editable: true
        }

        Label { text: i18n("Summary:") }
        TextField {
            id: summaryInput
            Layout.fillWidth: true
        }

        TextArea {
            id: reviewInput
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    standardButtons: StandardButton.Ok | StandardButton.Close
}
