/*
 *   Copyright (C) 2020 Aleix Pol Gonzalez <aleixpolkde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
