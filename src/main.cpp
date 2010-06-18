/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "MainWindow.h"

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>

static const char description[] =
    I18N_NOOP("A package manager built with QApt");

static const char version[] = "0.1";

int main(int argc, char **argv)
{
    KAboutData about("muon", 0, ki18n("Muon"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("(C) 2009, 2010 Jonathan Thomas"), KLocalizedString(), 0, "echidnaman@kubuntu.org");
    about.addAuthor( ki18n("Jonathan Thomas"), KLocalizedString(), "echidnaman@kubuntu.org" );
    about.setProgramIconName("application-x-deb");
    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    KCmdLineArgs::addCmdLineOptions(options);
    KApplication app;

    MainWindow *mainWindow = new MainWindow;

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(MainWindow);
    }
    else
    {
        mainWindow->show();
    }

    return app.exec();
}
