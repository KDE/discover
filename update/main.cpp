/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpolkde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "../DiscoverVersion.h"
#include "DiscoverUpdate.h"
#include <DiscoverBackendsFactory.h>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QGuiApplication>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("plasma-discover-update");
    KAboutData about(QStringLiteral("discoverupdate"), i18n("Discover Update"), version, {}, KAboutLicense::GPL, i18n("© 2020 Aleix Pol Gonzalez"), {});
    about.addAuthor(QStringLiteral("Aleix Pol i Gonzàlez"), {}, QStringLiteral("aleixpolkde.org"));
    about.setProductName("discover/update");
    KAboutData::setApplicationData(about);

    DiscoverUpdate exp;
    {
        QCommandLineParser parser;
        QCommandLineOption offlineUpdate(QStringLiteral("offline"), i18n("Prefer updates that will only apply upon reboot"));
        parser.addOption(offlineUpdate);
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser, false);

        exp.setOfflineUpdates(parser.isSet(offlineUpdate));
    }

    return app.exec();
}
