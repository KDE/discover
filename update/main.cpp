/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpolkde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>
#include <KLocalizedString>
#include <KAboutData>
#include "DiscoverUpdate.h"
#include <DiscoverBackendsFactory.h>
#include "../DiscoverVersion.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("plasma-discover-update");
    KAboutData about(QStringLiteral("discoverupdate"), i18n("Discover Update"), version, {},
                     KAboutLicense::GPL, i18n("© 2020 Aleix Pol Gonzalez"), {});
    about.addAuthor(QStringLiteral("Aleix Pol i Gonzàlez"), {}, QStringLiteral("aleixpolkde.org"));
    about.setProductName("discover/update");

    DiscoverUpdate exp;
    {
        QCommandLineParser parser;
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser, false);
    }

    return app.exec();
}
