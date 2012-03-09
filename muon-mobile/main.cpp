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
    I18N_NOOP("An application manager");

static const char version[] = "1.2.95 \"Daring Dalek\"";

int main(int argc, char** argv)
{
    KAboutData about("muon-installer", "muon-installer", ki18n("Muon Software Center"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("©2010, 2011 Jonathan Thomas"), KLocalizedString(), 0);
    about.addAuthor(ki18n("Jonathan Thomas"), KLocalizedString(), "echidnaman@kubuntu.org");
    about.addAuthor(ki18n("Aleix Pol Gonzalez"), KLocalizedString(), "aleixpol@blue-systems.com");
    about.setProgramIconName("applications-other");
    about.setProductName("muon/installer");

    KCmdLineArgs::init(argc, argv, &about);

    if (!KUniqueApplication::start()) {
        fprintf(stderr, "Software Center is already running!\n");
        return 0;
    }

    KUniqueApplication app;
    // Translations
    KGlobal::locale()->insertCatalog("app-install-data");
    KGlobal::locale()->insertCatalog("libmuon");
    // Needed for KIcon compatibility w/ application icons from app-install-data
    KGlobal::dirs()->addResourceDir("appicon", "/usr/share/app-install/icons/");
    app.disableSessionManagement();

    MuonInstallerMainWindow *mainWindow = new MuonInstallerMainWindow;
    mainWindow->show();

    return app.exec();
}
