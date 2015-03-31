/***************************************************************************
 *   Copyright © 2010-2013 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include <QApplication>
#include <KAboutData>
#include "../MuonVersion.h"
#include <KDBusService>
#include <KLocalizedString>
#include <QSessionManager>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme("muon"));
    KAboutData about("muon", i18n("Muon Package Manager"), version, i18n("A package manager"),
                     KAboutLicense::GPL, i18n("© 2009-2013 Jonathan Thomas"));
    about.addAuthor(i18n("Jonathan Thomas"), QString(), "echidnaman@kubuntu.org");
    about.setProductName("muon/muon");
    KAboutData::setApplicationData(about);

    {
        QCommandLineParser parser;
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
    }


    KDBusService service(KDBusService::Unique);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    MainWindow *mainWindow = new MainWindow;
    mainWindow->show();

    return app.exec();
}
