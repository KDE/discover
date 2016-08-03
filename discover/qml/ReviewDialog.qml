import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.OverlaySheet
{
    id: reviewDialog

    property QtObject application
    readonly property alias rating: ratingInput.rating
    readonly property alias summary: summaryInput.text
    readonly property alias review: reviewInput.text

    signal accepted()

    ColumnLayout {
        Heading { text: i18n("Reviewing '%1'", application.name) }
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

        Button {
            Layout.alignment: Qt.AlignRight
            text: i18n("Accept")
            onClicked: {
                reviewDialog.accepted()
                reviewDialog.opened = false
            }
        }
    }
}
