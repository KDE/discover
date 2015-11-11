/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
#include <klocalizedstring.h>
#include <KAboutData>
#include <QIcon>
#include "MuonExporter.h"
#include <DiscoverBackendsFactory.h>
#include "DiscoverVersion.h"

static const char description[] = I18N_NOOP("An application exporterer");

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("muon-exporter");
    KAboutData about(QStringLiteral("muonexporter"), i18n("Muon Exporter"), version, i18n(description),
                     KAboutLicense::GPL, i18n("©2013 Aleix Pol Gonzalez"), QString());
    about.addAuthor(i18n("Jonathan Thomas"), QString(), QStringLiteral("echidnaman@kubuntu.org"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@blue-systems.com"));
    about.setProductName("muon/exporter");

    MuonExporter exp;
    {
        QCommandLineParser parser;
        parser.addOption(QCommandLineOption(QStringLiteral("backends"), i18n("List all the backends we'll want to have loaded, separated by coma ','."), QStringLiteral("names")));
        parser.addPositionalArgument(QStringLiteral("file"), i18n("File to which we'll export"));
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.addHelpOption();
        parser.addVersionOption();
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser);

        if(parser.positionalArguments().count() != 1) {
            parser.showHelp(1);
        }
        exp.setExportPath(QUrl::fromUserInput(parser.positionalArguments().first(), QString(), QUrl::AssumeLocalFile));
    }

    QObject::connect(&exp, SIGNAL(exportDone()), &app, SLOT(quit()));

    return app.exec();
}
