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

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KCrash::setFlags(KCrash::AutoRestart);

    NotifierItem notifier;
    bool hide = false;
    KDBusService::StartupOptions startup = {};
    {
        KAboutData about(QStringLiteral("DiscoverNotifier"),
                         i18n("Discover Notifier"),
                         version,
                         i18n("System update status notifier"),
                         KAboutLicense::GPL,
                         i18n("© 2010-2022 Plasma Development Team"));
        about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
        about.setProductName("discover/discover");
        about.setProgramLogo(app.windowIcon());
        about.setTranslator(i18ndc(nullptr, "NAME OF TRANSLATORS", "Your names"), i18ndc(nullptr, "EMAIL OF TRANSLATORS", "Your emails"));

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
        KConfigGroup group(config, "Behavior");

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

    return app.exec();
}
