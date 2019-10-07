import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.5 as Kirigami
import org.kde.userfeedback 1.0 as UserFeedback
import "navigation.js" as Navigation

Kirigami.ApplicationWindow
{
    id: window
    readonly property string applicationListComp: ("qrc:/qml/ApplicationsListPage.qml")
    readonly property string applicationComp: ("qrc:/qml/ApplicationPage.qml")
    readonly property string reviewsComp: ("qrc:/qml/ReviewsPage.qml")

    //toplevels
    readonly property string topBrowsingComp: ("qrc:/qml/BrowsingPage.qml")
    readonly property string topInstalledComp: ("qrc:/qml/InstalledPage.qml")
    readonly property string topSearchComp: ("qrc:/qml/SearchPage.qml")
    readonly property string topUpdateComp: ("qrc:/qml/UpdatesPage.qml")
    readonly property string topSourcesComp: ("qrc:/qml/SourcesPage.qml")
    readonly property string topAboutComp: ("qrc:/qml/AboutPage.qml")
    readonly property string loadingComponent: ("qrc:/qml/LoadingPage.qml")
    readonly property QtObject stack: window.pageStack
    property string currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true

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

    Kirigami.Action {
        id: submitStatisticsAction
        text: i18n("Submit usage information")
        tooltip: i18n("Sends anonymized usage information to KDE so we can better understand our users. For more information see https://kde.org/privacypolicy-apps.php.")
        onTriggered: {
            provider.submit()
            showPassiveNotification(i18n("Submitting usage information..."), "short", i18n("Configure"), provider.encouraged)
        }
    }
    Kirigami.Action {
        id: feedbackSettingsAction
        text: i18n("Configure feedback...")
        onTriggered: {
            provider.encouraged()
        }
    }

    UserFeedback.Provider {
        id: provider

        submissionInterval: 7
        surveyInterval: 30
        feedbackServer: "https://telemetry.kde.org/"
        encouragementInterval: 30
        applicationStartsUntilEncouragement: 1
        applicationUsageTimeUntilEncouragement: 1
        telemetryMode: UserFeedbackSettings.feedbackLevel

        function encouraged() {
            KCMShell.open("kcm_feedback");
        }

        property var lastSurvey: null

        function openSurvey() {
            Qt.openUrlExternally(lastSurvey.url);
            surveyCompleted(lastSurvey);
        }

        onShowEncouragementMessage: {
            showPassiveNotification(i18n("You can help us improving this application by sharing statistics and participate in surveys."), 5000, i18n("Contribute..."), encouraged)
        }

        onSurveyAvailable: {
            lastSurvey = survey
            showPassiveNotification(i18n("We are looking for your feedback!"), 5000, i18n("Participate..."), openSurvey)
        }

        UserFeedback.ApplicationVersionSource { mode: UserFeedback.Provider.BasicSystemInformation }
        UserFeedback.PlatformInfoSource { mode: UserFeedback.Provider.BasicSystemInformation }
        UserFeedback.QtVersionSource { mode: UserFeedback.Provider.BasicSystemInformation }
        UserFeedback.StartCountSource { mode: UserFeedback.Provider.BasicUsageStatistics }
        UserFeedback.UsageTimeSource { mode: UserFeedback.Provider.BasicUsageStatistics }
        UserFeedback.LocaleInfoSource { mode: UserFeedback.Provider.DetailedSystemInformation }
        UserFeedback.OpenGLInfoSource{ mode: UserFeedback.Provider.DetailedSystemInformation }
        UserFeedback.ScreenInfoSource { mode: UserFeedback.Provider.DetailedSystemInformation }
        UserFeedback.PropertySource {
            id: dialRatioSource
            mode: UserFeedback.Provider.DetailedUsageStatistics
            sourceId: "applicationSourceName"
            data: { "value": ResourcesModel.applicationSourceName }
            description: "The source for applications"
        }
    }

    TopLevelPageData {
        iconName: "tools-wizard"
        text: i18n("Discover")
        component: topBrowsingComp
        objectName: "discover"
    }

    TopLevelPageData {
        id: searchAction
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
        text: ResourcesModel.updatesCount<=0 ? (ResourcesModel.isFetching ? i18n("Fetching Updates...") : i18n("Up to Date") ) : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
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
        tooltip: shortcut

        shortcut: "Ctrl+R"
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

        onOpenSearch: {
            Navigation.clearStack()
            Navigation.openApplicationList({search: search})
        }

        onOpenErrorPage: {
            Navigation.clearStack()
            console.warn("error", errorMessage)
            window.stack.push(errorPageComponent, { error: errorMessage, title: i18n("Sorry...") })
        }

        onPreventedClose: showPassiveNotification(i18n("Could not close Discover, there are tasks that need to be done."), 20000, i18n("Quit Anyway"), function() { Qt.quit() })
        onUnableToFind: {
            showPassiveNotification(i18n("Unable to find resource: %1", resid));
            Navigation.openHome()
        }
    }

    Connections {
        target: ResourcesModel
        onPassiveMessage: {
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
            Kirigami.Icon {
                id: infoIcon;
                anchors {
                    bottom: parent.verticalCenter
                    margins: Kirigami.Units.largeSpacing
                    horizontalCenter: parent.horizontalCenter
                }
                visible: page.error !== ""
                source: "emblem-warning"
                height: Kirigami.Units.iconSizes.huge
                width: height;
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

    Component {
        id: proceedDialog
        Kirigami.OverlaySheet {
            id: sheet
            showCloseButton: false
            property QtObject transaction
            property alias title: heading.text
            property alias description: desc.text
            property bool acted: false
            ColumnLayout {
                Kirigami.Heading {
                    id: heading
                }
                Label {
                    id: desc
                    Layout.fillWidth: true
                    textFormat: Text.StyledText
                    wrapMode: Text.WordWrap
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

    Instantiator {
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
