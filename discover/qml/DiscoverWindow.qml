import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.14
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.19 as Kirigami
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
    height: app.initialGeometry.height>=10 ? app.initialGeometry.height : Math.max(Kirigami.Units.gridUnit * 30, window.globalDrawer.contentHeight)

    visible: true

    minimumWidth: 300
    minimumHeight: 300

    pageStack.defaultColumnWidth: Math.max(Kirigami.Units.gridUnit * 25, pageStack.width / 4)
    pageStack.globalToolBar.style: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.Titles : Kirigami.ApplicationHeaderStyle.Auto
    pageStack.globalToolBar.showNavigationButtons: pageStack.currentIndex == 0 ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ShowBackButton
    pageStack.globalToolBar.canContainHandles: true // mobile handles in header

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
        iconName: "go-home-symbolic"
        text: i18n("&Home")
        component: topBrowsingComp
        objectName: "discover"
    }

    TopLevelPageData {
        id: searchAction
        visible: enabled
        enabled: !window.wideScreen
        iconName: "search"
        text: i18n("&Search")
        component: topSearchComp
        objectName: "search"
        shortcut: StandardKey.Find
    }
    TopLevelPageData {
        id: installedAction
        iconName: "view-list-details"
        text: i18n("&Installed")
        component: topInstalledComp
        objectName: "installed"
    }
    TopLevelPageData {
        id: updateAction
        iconName: ResourcesModel.updatesCount>0 ? ResourcesModel.hasSecurityUpdates ? "update-high" : "update-low" : "update-none"
        text: ResourcesModel.updatesCount<=0 ? (ResourcesModel.isFetching ? i18n("Fetching &updates…") : i18n("&Up to date") ) : i18nc("Update section name", "&Update (%1)", ResourcesModel.updatesCount)
        component: topUpdateComp
        objectName: "update"
    }
    TopLevelPageData {
        id: aboutAction
        iconName: "help-feedback"
        text: i18n("&About")
        component: topAboutComp
        objectName: "about"
        shortcut: StandardKey.HelpContents
    }
    TopLevelPageData {
        id: sourcesAction
        iconName: "configure"
        text: i18n("S&ettings")
        component: topSourcesComp
        objectName: "sources"
        shortcut: StandardKey.Preferences
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
        tooltip: shortcut.nativeText

        // Need to define an explicit Shortcut object so we can get its text
        // using shortcut.nativeText
        shortcut: Shortcut {
            sequences: [ StandardKey.Refresh ]
            onActivated: refreshAction.trigger()
        }
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

        function onOpenErrorPage(errorMessage, errorExplanation, buttonText, buttonIcon, buttonUrl) {
            Navigation.clearStack()
            console.warn("Error: " + errorMessage + "\n" + errorExplanation + "\n" + "Please visit " + buttonUrl)
            window.stack.push(errorPageComponent, { title: i18n("Error"), errorMessage: errorMessage, errorExplanation: errorExplanation, buttonText: buttonText, buttonIcon: buttonIcon, buttonUrl: buttonUrl })
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
        function onInlineMessage (type, icon, message) {
            leftPage.header.type = type
            leftPage.header.icon.name = icon
            leftPage.header.text = message
            leftPage.header.visible = true

            console.log("inline message:", message)
        }
    }

    footer: Loader {
        active: !window.wideScreen
        visible: active // ensure that no height is used when not loaded
        height: item ? item.implicitHeight : 0
        sourceComponent: Kirigami.NavigationTabBar {
            actions: [
                featuredAction,
                searchAction,
                installedAction,
                updateAction
            ]
            Component.onCompleted: {
                // Exclusivity is already handled by the actions. This prevents BUG:448460
                tabGroup.exclusive = false
            }
        }
    }

    Component {
        id: errorPageComponent
        Kirigami.Page {
            id: page
            property string errorMessage: ""
            property string errorExplanation: ""
            property string buttonText: ""
            property string buttonIcon: ""
            property string buttonUrl: ""
            readonly property bool isHome: true
            function searchFor(text) {
                if (text.length === 0)
                    return;
                Navigation.openCategory(null, "")
            }
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 8)
                visible: page.errorMessage !== ""
                icon.name: "emblem-warning"
                text: page.errorMessage
                explanation: page.errorExplanation
                helpfulAction: Kirigami.Action {
                    icon.name: page.buttonIcon
                    text: page.buttonText
                    onTriggered: {
                        Qt.openUrlExternally(page.buttonUrl)
                    }
                }
            }
        }
    }

    Component {
        id: proceedDialog
        Kirigami.OverlaySheet {
            id: sheet
            showCloseButton: false
            property QtObject transaction
            property alias description: desc.text
            property bool acted: false

            // No need to add our own ScrollView since OverlaySheet includes
            // one automatically.
            // But we do need to put the label into a Layout of some sort so we
            // can limit the width of the sheet.
            contentItem: ColumnLayout {
                Label {
                    id: desc

                    Layout.fillWidth: true
                    Layout.maximumWidth: Math.round(window.width * 0.75)

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
    Component {
        id: distroErrorMessageDialog
        Kirigami.OverlaySheet {
            id: sheet
            property alias message: desc.text
            Keys.onEscapePressed: sheetOpen = false

            // No need to add our own ScrollView since OverlaySheet includes
            // one automatically.
            // But we do need to put the label into a Layout of some sort so we
            // can limit the width of the sheet.
            contentItem: ColumnLayout {
                Label {
                    id: desc

                    Layout.fillWidth: true
                    Layout.maximumWidth: Math.round(window.width * 0.75)

                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Item { Layout.fillWidth : true }
                    Button {
                        icon.name: "tools-report-bug"
                        text: i18n("Report this issue")
                        onClicked: {
                            Qt.openUrlExternally(ResourcesModel.distroBugReportUrl())
                        }
                    }
                }
            }

            onSheetOpenChanged: if(!sheetOpen) {
                sheet.destroy(1000)
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
                app.restore()
            }
            function onPassiveMessage(message) {
                window.showPassiveNotification(message)
                app.restore()
            }
            function onDistroErrorMessage(message, actions) {
                var dialog = distroErrorMessageDialog.createObject(window, {transaction: transaction, title: i18n("Error"), message: message})
                dialog.open()
                app.restore()
            }
            function onWebflowStarted(url) {
                var component = Qt.createComponent("WebflowDialog.qml");
                if (component.status == Component.Error) {
                    Qt.openUrlExternally(url);
                    console.error("Webflow Error", component.errorString())
                } else if (component.status == Component.Ready) {
                    const sheet = component.createObject(window, {transaction: transaction, url: url });
                    sheet.open()
                }
            }
        }
    }

    PowerManagementInterface {
        reason: TransactionModel.mainTransactionText
        preventSleep: TransactionModel.count > 0
    }

    contextDrawer: Kirigami.ContextDrawer {}

    globalDrawer: DiscoverDrawer {
        wideScreen: window.wideScreen
    }

    onCurrentTopLevelChanged: {
        window.pageStack.clear()
        if (currentTopLevel)
            window.pageStack.push(currentTopLevel, {}, window.status!==Component.Ready)
        globalDrawer.forceSearchFieldFocus();
    }

    UnityLauncher {
        launcherId: "org.kde.discover.desktop"
        progressVisible: TransactionModel.count > 0
        progress: TransactionModel.progress
    }
}
