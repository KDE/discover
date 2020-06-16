import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.13 as Kirigami

Kirigami.RouterWindow {
    id: window

    globalDrawer: DiscoverDrawer {
        Kirigami.PageRouter.router: window.router
        wideScreen: window.wideScreen
    }

    initialRoute: "browsing"

    Kirigami.PageRoute {
        name: "sources"
        cache: true
        SourcesPage {}
    }

    Kirigami.PageRoute {
        name: "installed"
        cache: true
        InstalledPage {}
    }

    Kirigami.PageRoute {
        name: "update"
        cache: true
        UpdatesPage {}
    }

    Kirigami.PageRoute {
        name: "about"
        cache: true
        AboutPage {}
    }

    Kirigami.PageRoute {
        name: "browsing"
        cache: true
        BrowsingPage {}
    }

    Kirigami.PageRoute {
        name: "application"
        cache: true
        ApplicationPage {}
    }

    Kirigami.PageRoute {
        name: "application-list"
        cache: true
        ApplicationsListPage {}
    }

    Kirigami.PageRoute {
        name: "error"
        Kirigami.Page {
            id: page
            property string error: Kirigami.PageRouter.data
            title: i18n("Sorry...")
            readonly property bool isHome: true
            Kirigami.Icon {
                id: infoIcon
                anchors {
                    bottom: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                    horizontalCenter: parent.horizontalCenter
                }
                visible: page.error !== ""
                source: "emblem-warning"
                height: Kirigami.Units.iconSizes.huge
                width: height
            }
            Kirigami.Heading {
                anchors {
                    top: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width;
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                visible: page.error !== ""
                text: page.error
            }
        }
    }

    width: app.initialGeometry.width>=10 ? app.initialGeometry.width : Kirigami.Units.gridUnit * 45
    height: app.initialGeometry.height>=10 ? app.initialGeometry.height : Kirigami.Units.gridUnit * 30
    minimumWidth: 300
    minimumHeight: 300

    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 25
    pageStack.globalToolBar.style: window.wideScreen ? Kirigami.ApplicationHeaderStyle.ToolBar : Kirigami.ApplicationHeaderStyle.Breadcrumb

    Component.onCompleted: {
        if (app.isRoot)
            showPassiveNotification(i18n("Running as <em>root</em> is discouraged and unnecessary."));
    }

    readonly property string describeSources: feedbackLoader.item ? feedbackLoader.item.describeDataSources : ""
    property Loader loader: Loader {
        id: feedbackLoader
        source: "Feedback.qml"
    }

    property Kirigami.Action refreshAction: Kirigami.Action {
        id: refreshAction
        readonly property QtObject action: ResourcesModel.updateAction
        text: action.text
        icon.name: "view-refresh"
        onTriggered: action.trigger()
        enabled: action.enabled
        tooltip: shortcut

        shortcut: "Ctrl+R"
    }

    property Connections appConnections: Connections {
        target: app
        function onOpenApplicationInternal(app) {
            Kirigami.PageRouter.pushFromHere({"route": "application", "data": app})
        }
        function onListMimeInternal(mime) {
            Kirigami.PageRouter.navigateToRoute({"route": "application-list", "mimeTypeFilter": mime, "title": i18n("Resources for '%1'", mime) })
        }
        function onListCategoryInternal(cat)  {
            Kirigami.PageRouter.navigateToRoute({"route": "application-list", "category": cat, "search": "" })
        }
        function onOpenSearch(search) {
            Kirigami.PageRouter.navigateToRoute({"route": "application-list", "search": "" })
        }
        function onOpenErrorPage(errorMessage) {
            console.warn("error", errorMessage)
            Kirigami.PageRouter.navigateToRoute({"route": "error", "data": errorMessage})
        }
        function onPreventedClose() {
            showPassiveNotification(i18n("Could not close Discover, there are tasks that need to be done."), 20000, i18n("Quit Anyway"), function() { Qt.quit() })
        }
        function onUnableToFind(resid) {
            showPassiveNotification(i18n("Unable to find resource: %1", resid));
            Kirigami.PageRouter.navigateToRoute("browsing")
        }
    }

    readonly property Connections resourcesModelConnections: Connections {
        target: ResourcesModel
        function onPassiveMessage (message) {
            showPassiveNotification(message)
            console.log("message:", message)
        }
    }

    readonly property Component proceedDialogComponent: Component {
        id: proceedDialog
        Kirigami.OverlaySheet {
            id: sheet
            showCloseButton: false
            property QtObject transaction
            property alias title: heading.text
            property alias description: desc.text
            property bool acted: false

            header: Kirigami.Heading {
                id: heading
                wrapMode: Text.WordWrap
            }

            ColumnLayout {
                Label {
                    id: desc
                    Layout.fillWidth: true
                    Layout.maximumHeight: clip ? window.height * 0.5 : implicitHeight
                    clip: true
                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap

                    readonly property var bottomShadow: Shadow {
                        parent: desc
                        anchors {
                            right: parent.right
                            left: parent.left
                            bottom: parent.bottom
                        }
                        visible: desc.clip
                        edge: Qt.BottomEdge
                        height: desc.height * 0.01
                    }
                }
                Button {
                    text: desc.clip ? i18n("Show all") : i18n("Hide")
                    onClicked: desc.clip = !desc.clip
                    visible: window.height * 0.5 < desc.implicitHeight
                }
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    Button {
                        text: i18n("Proceed")
                        icon.name: "dialog-ok"
                        onClicked: {
                            transaction.proceed()
                            sheet.acted = true
                            sheet.close()
                        }
                        Keys.onEnterPressed: clicked()
                        Keys.onReturnPressed: clicked()
                    }
                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                        onClicked: {
                            transaction.cancel()
                            sheet.acted = true
                            sheet.close()
                        }
                        Keys.onEscapePressed: clicked()
                    }
                }
            }
            onSheetOpenChanged: if(!sheetOpen) {
                sheet.destroy(1000)
                if (!sheet.acted)
                    transaction.cancel()
            }
        }
    }

    readonly property Instantiator instantiator: Instantiator {
        model: TransactionModel

        delegate: Connections {
            target: model.transaction ? model.transaction : null

            onProceedRequest: {
                var dialog = proceedDialog.createObject(window, {transaction: transaction, title: title, description: description})
                dialog.open()
            }
            onPassiveMessage: {
                window.showPassiveNotification(message)
            }
        }
    }

    readonly property ConditionalObject conditionalObject: ConditionalObject {
        id: drawerObject
        condition: window.wideScreen
        componentFalse: Kirigami.ContextDrawer {}
    }
    contextDrawer: drawerObject.object

    readonly property UnityLauncher unityLauncher: UnityLauncher {
        launcherId: "org.kde.discover.desktop"
        progressVisible: TransactionModel.count > 0
        progress: TransactionModel.progress
    }
}
