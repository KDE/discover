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
#include "MuonInstallerMainWindow.h"

static const char description[] =
    I18N_NOOP("An application discoverer");

static const char version[] = "1.2.95 \"Daring Dalek\"";

int main(int argc, char** argv)
{
    KAboutData about("muon-discover", "muon-discover", ki18n("Muon Discover"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("©2010, 2011 Jonathan Thomas"), KLocalizedString(), 0);
    about.addAuthor(ki18n("Jonathan Thomas"), KLocalizedString(), "echidnaman@kubuntu.org");
    about.addAuthor(ki18n("Aleix Pol Gonzalez"), KLocalizedString(), "aleixpol@blue-systems.com");
    about.setProgramIconName("muondiscover");
    about.setProductName("muon/discover");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineOptions options;
    options.add("application <name>", ki18n("Directly open the specified application by its package name."));
    options.add("mime <name>", ki18n("Open with a program that can deal with the given mimetype."));
    KCmdLineArgs::addCmdLineOptions( options );

    if (!KUniqueApplication::start()) {
        fprintf(stderr, "Software Discoverer is already running!\n");
        return 0;
    }

    KUniqueApplication app;
    // Translations
    KGlobal::locale()->insertCatalog("app-install-data");
    KGlobal::locale()->insertCatalog("libmuon");
    // Needed for KIcon compatibility w/ application icons from app-install-data
    KGlobal::dirs()->addResourceDir("appicon", "/usr/share/app-install/icons/");
    app.disableSessionManagement();
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    
    MuonInstallerMainWindow *mainWindow = new MuonInstallerMainWindow;
    if(args->isSet("application"))
        mainWindow->openApplication(args->getOption("application"));
    else if(args->isSet("mime"))
        mainWindow->openMimeType(args->getOption("mime"));
    mainWindow->show();

    return app.exec();
}
