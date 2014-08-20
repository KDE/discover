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
#include <KAboutData>
#include <kdbusservice.h>
#include <QCommandLineParser>
#include "MuonDiscoverMainWindow.h"
#include <MuonBackendsFactory.h>
#include "MuonVersion.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KAboutData about("muon-discover", i18n("Muon Discover"), version, i18n("An application discoverer"),
                     KAboutLicense::GPL, i18n("©2010-2012 Jonathan Thomas"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), "aleixpol@blue-systems.com");
    about.addAuthor(i18n("Jonathan Thomas"), QString(), "echidnaman@kubuntu.org");
    about.setProgramIconName("muondiscover");
    about.setProductName("muon/discover");
    app.setApplicationDisplayName("Muon Discover");
    app.setApplicationName("muon-discover");
    app.setApplicationVersion(version);

//     KDBusService service(KDBusService::Unique);
    MuonDiscoverMainWindow *mainWindow = new MuonDiscoverMainWindow;
    QObject::connect(&app, SIGNAL(aboutToQuit()), mainWindow, SLOT(deleteLater()));
    {
        QCommandLineParser parser;
        parser.addOption(QCommandLineOption("application", i18n("Directly open the specified application by its package name."), "name"));
        parser.addOption(QCommandLineOption("mime", i18n("Open with a program that can deal with the given mimetype."), "name"));
        parser.addOption(QCommandLineOption("category", i18n("Display a list of entries with a category."), "name"));
        parser.addOption(QCommandLineOption("mode", i18n("Open Muon Discover in a said mode. Modes correspond to the toolbar buttons.")));
        parser.addOption(QCommandLineOption("listmodes", i18n("List all the available modes."), "name"));
        parser.addOption(QCommandLineOption("listbackends", i18n("List all the available backends.")));
        parser.addOption(QCommandLineOption("backends", i18n("List all the backends we'll want to have loaded, separated by coma ','."), "names"));
        parser.addHelpOption();
        parser.addVersionOption();
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        MuonBackendsFactory::setRequestedBackends(parser.value("backends").split(",", QString::SkipEmptyParts));

        if(parser.isSet("application"))
            mainWindow->openApplication(parser.value("application"));
        else if(parser.isSet("mime"))
            mainWindow->openMimeType(parser.value("mime"));
        else if(parser.isSet("category"))
            mainWindow->openCategory(parser.value("category"));
        else if(parser.isSet("mode"))
            mainWindow->openMode(parser.value("mode").toLocal8Bit());
        else if(parser.isSet("listmodes")) {
            fprintf(stdout, "%s", qPrintable(i18n("Available modes:\n")));
            foreach(const QString& mode, mainWindow->modes())
                fprintf(stdout, " * %s\n", qPrintable(mode));
            return 0;
        } else if(parser.isSet("listbackends")) {
            fprintf(stdout, "%s", qPrintable(i18n("Available backends:\n")));
            MuonBackendsFactory f;
            foreach(const QString& name, f.allBackendNames())
                fprintf(stdout, " * %s\n", qPrintable(name));
            return 0;
        }
    }

    mainWindow->show();

    return app.exec();
}
