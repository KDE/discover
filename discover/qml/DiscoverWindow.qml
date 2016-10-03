import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.ApplicationWindow
{
    id: window
    readonly property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    readonly property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    readonly property Component reviewsComp: Qt.createComponent("qrc:/qml/ReviewsPage.qml")

    //toplevels
    readonly property Component topBrowsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    readonly property Component topInstalledComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    readonly property Component topUpdateComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    readonly property Component topSourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    readonly property QtObject stack: window.pageStack
    property Component currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true

    objectName: "DiscoverMainWindow"
    title: leftPage ? leftPage.title : ""

    header: null
    visible: true

    minimumWidth: 300
    minimumHeight: 300

    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 25

    readonly property var leftPage: window.stack.depth>0 ? window.stack.get(0) : null

    Component.onCompleted: {
        Helpers.mainWindow = window
        if (app.isRoot)
            showPassiveNotification(i18n("Running as <em>root</em> is discouraged and unnecessary."));
    }

    function clearSearch() {
        if (loader.item)
            loader.item.clearSearch();
    }

    Component {
        id: loadingComponent
        Kirigami.Page {
            title: label.text
            Label {
                id: label
                text: i18n("Loading...")
                font.pointSize: 52
                anchors.centerIn: parent
            }
        }
    }

    ExclusiveGroup { id: appTabs }

    property list<Action> awesome: [
        TopLevelPageData {
            iconName: "tools-wizard"
            text: i18n("Discover")
            component: topBrowsingComp
            objectName: "discover"
            shortcut: "Alt+D"
        }
    ]
    TopLevelPageData {
        id: installedAction
        text: TransactionModel.count == 0 ? i18n("Installed") : i18n("Installing...")
        component: topInstalledComp
        objectName: "installed"
        shortcut: "Alt+I"
    }
    TopLevelPageData {
        id: updateAction
        iconName: enabled ? "update-low" : "update-none"
        text: !enabled ? i18n("No Updates") : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
        enabled: ResourcesModel.updatesCount>0
        component: topUpdateComp
        objectName: "update"
        shortcut: "Alt+U"
    }
    TopLevelPageData {
        id: settingsAction
        iconName: "settings"
        text: i18n("Settings")
        component: topSourcesComp
        objectName: "settings"
        shortcut: "Alt+S"
    }
    TopLevelPageData {
        id: sources
        text: i18n("Configure Sources...")
        iconName: "repository"
        shortcut: "Alt+S"
        component: topSourcesComp
    }

    Connections {
        target: app
        onOpenApplicationInternal: {
            Navigation.clearStack()
            Navigation.openApplication(app)
        }
        onListMimeInternal:  {
            currentTopLevel = topBrowsingComp;
            Navigation.openApplicationMime(mime)
        }
        onListCategoryInternal:  {
            currentTopLevel = topBrowsingComp;
            Navigation.openCategory(cat, "")
        }

        onPreventedClose: showPassiveNotification(i18n("Could not close the application, there are tasks that need to be done."), 3000)
        onUnableToFind: {
            showPassiveNotification(i18n("Unable to find resource: %1", resid));
            Navigation.openHome()
        }
    }

    Component {
        id: proceedDialog
        Kirigami.OverlaySheet {
            id: sheet
            property QtObject transaction
            property alias title: heading.text
            property alias description: desc.text
            property bool acted: false
            ColumnLayout {
                Kirigami.Heading {
                    id: heading
                }
                Kirigami.Label {
                    id: desc
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Button {
                    Layout.alignment: Qt.AlignRight
                    iconName: "dialog-ok"
                    onClicked: {
                        transaction.proceed()
                        sheet.acted = true
                        sheet.close()
                    }
                }
            }
            onVisibleChanged: if(!visible) {
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

            onProceedRequest: {
                var dialog = proceedDialog.createObject(window, {transaction: transaction, title: title, description: description})
                dialog.open()
            }
        }
    }

    globalDrawer: DiscoverDrawer {}

    onCurrentTopLevelChanged: {
        if(currentTopLevel && currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        var stackView = window.pageStack;
        stackView.clear()
        if (currentTopLevel)
            stackView.push(currentTopLevel, {}, window.status!=Component.Ready)
    }

    Menu {
        id: actionsMenu
    }

    Instantiator {
        model: MessageActionsModel {}
        delegate: MenuItem {
            action: ActionBridge { action: model.action }
        }
        onObjectAdded: {
            actionsMenu.insertItem(index, object)
        }
        onObjectRemoved: {
            object.destroy()
        }
    }

    UnityLauncher {
        launcherId: "org.kde.discover.desktop"
        progressVisible: TransactionModel.count > 0
        progress: TransactionModel.progress
    }
}
