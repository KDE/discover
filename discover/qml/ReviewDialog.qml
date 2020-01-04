import QtQuick 2.3
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import org.kde.kirigami 2.10 as Kirigami

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
        Layout.maximumWidth: Kirigami.Units.gridUnit * 8

        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
            wrapMode: Text.WordWrap
            level: 2
            text: i18n("Reviewing %1", application.name)
        }

        Kirigami.FormLayout {
            id: contentLayout
            Layout.fillHeight: true

            Rating {
                id: ratingInput
                Kirigami.FormData.label: i18n("Rating:")
                editable: true
            }
            Label {
                Kirigami.FormData.label: i18n("Name:")
                visible: reviewDialog.backend.userName.length > 0
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: visible ? reviewDialog.backend.userName : ""
            }
            TextField {
                id: titleInput
                Kirigami.FormData.label: i18n("Title:")
                Layout.fillWidth: true
                validator: RegExpValidator { regExp: /.{3,70}/ }
            }
        }

        TextArea {
            id: reviewInput
            readonly property bool acceptableInput: length > 15 && length < 3000
            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 8
        }

        RowLayout {

            Label {
                id: instructionalLabel
                Layout.fillWidth: true
                text: {
                    if (rating < 2) return i18n("Enter a rating");
                    if (! titleInput.acceptableInput) return i18n("Write the title");
                    if (reviewInput.length === 0) return i18n("Write the review");
                    if (reviewInput.length < 15) return i18n("Keep writing...");
                    if (reviewInput.length > 3000) return i18n("Too long!");
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
            Button {
                id: acceptButton
                enabled: !instructionalLabel.visible
                text: i18n("Submit review")
                onClicked: {
                    reviewDialog.accepted()
                    reviewDialog.sheetOpen = false
                }
            }
        }
    }
}
