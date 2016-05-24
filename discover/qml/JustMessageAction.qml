import QtQuick.Controls 1.2

MessageAction {
    property alias message: action.tooltip
    property alias enabled: action.enabled

    theAction: Action {
        id: action
        enabled: false
        text: i18n("Got it");
        onTriggered: { enabled=false }
    }
}
