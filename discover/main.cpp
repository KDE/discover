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

#include <KApplication>
#include <QWidget>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KUniqueApplication>
#include <KStandardDirs>
#include "MuonDiscoverMainWindow.h"

static const char description[] =
    I18N_NOOP("An application discoverer");

static const char version[] = "1.9.80";

int main(int argc, char** argv)
{
    KAboutData about("muon-discover", "muon-discover", ki18n("Muon Discover"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("©2010-2012 Jonathan Thomas"), KLocalizedString(), 0);
    about.addAuthor(ki18n("Jonathan Thomas"), KLocalizedString(), "echidnaman@kubuntu.org");
    about.addAuthor(ki18n("Aleix Pol Gonzalez"), KLocalizedString(), "aleixpol@blue-systems.com");
    about.setProgramIconName("muondiscover");
    about.setProductName("muon/discover");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineOptions options;
    options.add("application <name>", ki18n("Directly open the specified application by its package name."));
    options.add("mime <name>", ki18n("Open with a program that can deal with the given mimetype."));
    options.add("category <name>", ki18n("Display a list of entries with a category."));
    options.add("mode <name>", ki18n("Open Muon Discover in a said mode. Modes correspond to the toolbar buttons."));
    options.add("listmodes", ki18n("List all the available modes and output them on stdout."));
    KCmdLineArgs::addCmdLineOptions( options );

    if (!KUniqueApplication::start()) {
        fprintf(stderr, "Software Discoverer is already running!\n");
        return 0;
    }

    KUniqueApplication app;
    app.disableSessionManagement();
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    
    MuonDiscoverMainWindow *mainWindow = new MuonDiscoverMainWindow;
    if(args->isSet("application"))
        mainWindow->openApplication(args->getOption("application"));
    else if(args->isSet("mime"))
        mainWindow->openMimeType(args->getOption("mime"));
    else if(args->isSet("category"))
        mainWindow->openCategory(args->getOption("category"));
    else if(args->isSet("mode"))
        mainWindow->openMode(args->getOption("mode").toLocal8Bit());
    else if(args->isSet("listmodes")) {
        fprintf(stdout, "%s", qPrintable(i18n("Available modes:\n")));
        foreach(const QString& mode, mainWindow->modes())
            fprintf(stdout, " * %s\n", qPrintable(mode));
        return 0;
    }
    mainWindow->show();

    return app.exec();
}
