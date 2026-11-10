import QtQml
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.userfeedback as UserFeedback
import org.kde.ki18n
import org.kde.kcmutils as KCMUtils
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

UserFeedback.Provider {
    id: provider

    readonly property list<T.Action> actions: [
        Kirigami.Action {
            text: KI18n.i18nc("@action:inmenu", "Submit Usage Information")
            tooltip: KI18n.i18nc("@info:tooltip", "Sends anonymized usage information to KDE so we can better understand our users. For more information see https://kde.org/privacypolicy-apps.php.")
            icon.name: "document-send"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                provider.submit()
                showPassiveNotification(KI18n.i18nc("@info status passive notification", "Submitting usage information…"),
                                        "short",
                                        KI18n.i18nc("@action:button on passive notification, opens feedback kcm", "Configure"),
                                        provider.encouraged)
            }
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:inmenu", "Configure Feedback…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                provider.encouraged()
            }
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:button", "Configure Updates…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            icon.name: "configure"
            onTriggered: {
                KCMUtils.KCMLauncher.openSystemSettings("kcm_updates");
            }
        }
    ]

    submissionInterval: 7
    surveyInterval: -1
    feedbackServer: "https://telemetry.kde.org/"
    encouragementInterval: 30
    applicationStartsUntilEncouragement: 1
    applicationUsageTimeUntilEncouragement: 1
    telemetryMode: DiscoverApp.UserFeedbackSettings.feedbackLevel

    function encouraged() {
        KCMUtils.KCMLauncher.openSystemSettings("kcm_feedback");
    }

    property var lastSurvey: null

    function openSurvey() {
        Qt.openUrlExternally(lastSurvey.url);
        surveyCompleted(lastSurvey);
    }

    onShowEncouragementMessage: {
        showPassiveNotification(KI18n.i18nc("@info:usagetip passive notification", "You can help KDE improve Discover by sharing anonymous information on how you use it."),
                                5000,
                                KI18n.i18nc("@action:button on passive notification, opens feedback kcm", "Contribute…"),
                                encouraged)
    }

    onSurveyAvailable: {
        lastSurvey = survey
        showPassiveNotification(KI18n.i18nc("@info passive notification", "We are looking for your feedback!"),
                                5000,
                                KI18n.i18nc("@action:button on passive notification, opens url in browser", "Participate…"),
                                openSurvey)
    }

    UserFeedback.ApplicationVersionSource { mode: UserFeedback.Provider.BasicSystemInformation }
    UserFeedback.PlatformInfoSource { mode: UserFeedback.Provider.BasicSystemInformation }
    UserFeedback.QtVersionSource { mode: UserFeedback.Provider.BasicSystemInformation }
    UserFeedback.StartCountSource { mode: UserFeedback.Provider.BasicUsageStatistics }
    UserFeedback.UsageTimeSource { mode: UserFeedback.Provider.BasicUsageStatistics }
    UserFeedback.LocaleInfoSource { mode: UserFeedback.Provider.DetailedSystemInformation }
    UserFeedback.OpenGLInfoSource { mode: UserFeedback.Provider.DetailedSystemInformation }
    UserFeedback.ScreenInfoSource { mode: UserFeedback.Provider.DetailedSystemInformation }
    UserFeedback.PropertySource {
        mode: UserFeedback.Provider.DetailedUsageStatistics
        name: "Application Source Name"
        sourceId: "applicationSourceName"
        data: { "value": Discover.ResourcesModel.applicationSourceName }
        description: "The source for applications"
    }
}
