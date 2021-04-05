import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.14
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.14 as Kirigami
import "navigation.js" as Navigation

Kirigami.ApplicationWindow
{
    id: window
    readonly property string topBrowsingComp: ("qrc:/qml/BrowsingPage.qml")
    readonly property string topInstalledComp: ("qrc:/qml/InstalledPage.qml")
    readonly property string topSearchComp: ("qrc:/qml/SearchPage.qml")
    readonly property string topUpdateComp: ("qrc:/qml/UpdatesPage.qml")
    readonly property string topSourcesComp: ("qrc:/qml/SourcesPage.qml")
    readonly property string topAboutComp: ("qrc:/qml/AboutPage.qml")
    readonly property QtObject stack: window.pageStack
    property string currentTopLevel

    objectName: "DiscoverMainWindow"
    title: leftPage ? leftPage.title : ""

    width: app.initialGeometry.width>=10 ? app.initialGeometry.width : Kirigami.Units.gridUnit * 45
    height: app.initialGeometry.height>=10 ? app.initialGeometry.height : Kirigami.Units.gridUnit * 30

    visible: true

    minimumWidth: 300
    minimumHeight: 300

    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 25
    pageStack.globalToolBar.style: window.wideScreen ? Kirigami.ApplicationHeaderStyle.ToolBar : Kirigami.ApplicationHeaderStyle.Breadcrumb

    readonly property var leftPage: window.stack.depth>0 ? window.stack.get(0) : null

    Component.onCompleted: {
        if (app.isRoot)
            showPassiveNotification(i18n("Running as <em>root</em> is discouraged and unnecessary."));
    }

    readonly property string describeSources: feedbackLoader.item ? feedbackLoader.item.describeDataSources : ""
    Loader {
        id: feedbackLoader
        source: "Feedback.qml"
    }

    TopLevelPageData {
        id: featuredAction
        iconName: "go-home"
        text: i18n("Home")
        enabled: !window.wideScreen
        component: topBrowsingComp
        objectName: "discover"
    }

    TopLevelPageData {
        id: searchAction
        visible: enabled
        enabled: !window.wideScreen
        iconName: "search"
        text: i18n("Search")
        component: topSearchComp
        objectName: "discover"
        shortcut: "Ctrl+F"
    }
    TopLevelPageData {
        id: installedAction
        iconName: "view-list-details"
        text: i18n("Installed")
        component: topInstalledComp
        objectName: "installed"
    }
    TopLevelPageData {
        id: updateAction
        iconName: ResourcesModel.updatesCount>0 ? ResourcesModel.hasSecurityUpdates ? "update-high" : "update-low" : "update-none"
        text: ResourcesModel.updatesCount<=0 ? (ResourcesModel.isFetching ? i18n("Fetching updates...") : i18n("Up to date") ) : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
        component: topUpdateComp
        objectName: "update"
    }
    TopLevelPageData {
        id: aboutAction
        iconName: "help-feedback"
        text: i18n("About")
        component: topAboutComp
        objectName: "about"
    }
    TopLevelPageData {
        id: sourcesAction
        iconName: "configure"
        text: i18n("Settings")
        component: topSourcesComp
        objectName: "sources"
    }

    Kirigami.Action {
        id: refreshAction
        readonly property QtObject action: ResourcesModel.updateAction
        text: action.text
        icon.name: "view-refresh"
        onTriggered: action.trigger()
        enabled: action.enabled
        // Don't need to show this action in mobile view since you can pull down
        // on the view to refresh, and this is the common and expected behavior
        //on that platform
        visible: window.wideScreen
        tooltip: shortcut

        shortcut: "Ctrl+R"
    }

    Connections {
        target: app
        function onOpenApplicationInternal(app) {
            Navigation.clearStack()
            Navigation.openApplication(app)
        }
        function onListMimeInternal(mime)  {
            currentTopLevel = topBrowsingComp;
            Navigation.openApplicationMime(mime)
        }
        function onListCategoryInternal(cat)  {
            currentTopLevel = topBrowsingComp;
            Navigation.openCategory(cat, "")
        }

        function onOpenSearch(search) {
            Navigation.clearStack()
            Navigation.openApplicationList({search: search})
        }

        function onOpenErrorPage(errorMessage) {
            Navigation.clearStack()
            console.warn("error", errorMessage)
            window.stack.push(errorPageComponent, { error: errorMessage, title: i18n("Error") })
        }

        function onPreventedClose() {
            showPassiveNotification(i18n("Could not close Discover, there are tasks that need to be done."), 20000, i18n("Quit Anyway"), function() { Qt.quit() })
        }
        function onUnableToFind(resid) {
            showPassiveNotification(i18n("Unable to find resource: %1", resid));
            Navigation.openHome()
        }
    }

    Connections {
        target: ResourcesModel
        function onPassiveMessage (message) {
            showPassiveNotification(message)
            console.log("message:", message)
        }
    }

    Component {
        id: errorPageComponent
        Kirigami.Page {
            id: page
            property string error: ""
            readonly property bool isHome: true
            function searchFor(text) {
                if (text.length === 0)
                    return;
                Navigation.openCategory(null, "")
            }
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 8)
                visible: page.error !== ""
                icon.name: "error"
                text: page.error
            }
        }
    }

    Component {
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

            // No need to add our own ScrollView since OverlaySheet includes
            // one automatically.
            // But we do need to put the label into a Layout of some sort so we
            // can limit the width of the sheet.
            contentItem: ColumnLayout {
                Label {
                    id: desc

                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 30

                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap
                }
            }

            footer: RowLayout {

                Item { Layout.fillWidth : true }

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

            onSheetOpenChanged: if(!sheetOpen) {
                sheet.destroy(1000)
                if (!sheet.acted)
                    transaction.cancel()
            }
        }
    }

    Instantiator {
        model: TransactionModel

        delegate: Connections {
            target: model.transaction ? model.transaction : null

            function onProceedRequest(title, description) {
                var dialog = proceedDialog.createObject(window, {transaction: transaction, title: title, description: description})
                dialog.open()
            }
            function onPassiveMessage(message) {
                window.showPassiveNotification(message)
            }
        }
    }

    ConditionalObject {
        id: drawerObject
        condition: window.wideScreen
        componentFalse: Kirigami.ContextDrawer {}
    }
    contextDrawer: drawerObject.object

    globalDrawer: DiscoverDrawer {
        wideScreen: window.wideScreen
    }

    onCurrentTopLevelChanged: {
        window.pageStack.clear()
        if (currentTopLevel)
            window.pageStack.push(currentTopLevel, {}, window.status!==Component.Ready)
    }

    UnityLauncher {
        launcherId: "org.kde.discover.desktop"
        progressVisible: TransactionModel.count > 0
        progress: TransactionModel.progress
    }
}
