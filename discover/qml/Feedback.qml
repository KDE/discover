import org.kde.kirigami 2.5 as Kirigami
import org.kde.userfeedback 1.0 as UserFeedback
import org.kde.discover.app 1.0
import QtQml 2.0

UserFeedback.Provider
{
    readonly property list<QtObject> actions: [
        Kirigami.Action {
            text: i18n("Submit usage information")
            tooltip: i18n("Sends anonymized usage information to KDE so we can better understand our users. For more information see https://kde.org/privacypolicy-apps.php.")
            onTriggered: {
                provider.submit()
                showPassiveNotification(i18n("Submitting usage information..."), "short", i18n("Configure"), provider.encouraged)
            }
        },
        Kirigami.Action {
            text: i18n("Configure feedback...")
            onTriggered: {
                provider.encouraged()
            }
        }
    ]

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
        mode: UserFeedback.Provider.DetailedUsageStatistics
        sourceId: "applicationSourceName"
        data: { "value": ResourcesModel.applicationSourceName }
        description: "The source for applications"
    }
}
