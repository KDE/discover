/*
 *   SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "../DiscoverVersion.h"
#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QMenu>

#include "NotifierItem.h"

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    KCrash::setFlags(KCrash::AutoRestart);

    NotifierItem notifier;
    bool hide = false;
    KDBusService::StartupOptions startup = {};
    {
        KAboutData about(QStringLiteral("discover.notifier"),
                         i18n("Discover Notifier"),
                         version,
                         i18n("System update status notifier"),
                         KAboutLicense::GPL,
                         i18n("© 2010-2022 Plasma Development Team"));
        about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
        about.setProductName("discover/discover");
        about.setProgramLogo(app.windowIcon());
        about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

        KAboutData::setApplicationData(about);

        QCommandLineParser parser;
        QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
        parser.addOption(replaceOption);
        QCommandLineOption hideOption({QStringLiteral("hide")}, i18n("Do not show the notifier"), i18n("hidden"), QStringLiteral("false"));
        parser.addOption(hideOption);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);

        if (parser.isSet(replaceOption)) {
            startup |= KDBusService::Replace;
        }

        const auto config = KSharedConfig::openConfig();
        KConfigGroup group(config, u"Behavior"_s);

        if (parser.isSet(hideOption)) {
            hide = parser.value(hideOption) == QLatin1String("true");
            group.writeEntry<bool>("Hide", hide);
            config->sync();
        } else {
            hide = group.readEntry<bool>("Hide", false);
        }
    }

    KDBusService service(KDBusService::Unique | startup);
    notifier.setVisible(!hide);

    // Just quit if the notification frequency is set to never
    // The downside is that updates to the notification frequency will only take
    // effect after reboot
    // FIXME: more graceful handeling of notification frequency changes, while
    // making sure the notifier does not run when the frequency is set to never
    if (!notifier.isEnabled()) {
        return 0;
    }

    return app.exec();
}
