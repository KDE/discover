/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QProcess>
#include <KService>
#include <KIO/DesktopExecParser>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2)
        return 1;

    KService _service(app.arguments().last());
    if (!_service.isValid())
        return 2;

    QTextStream cerr(stderr);
    KIO::DesktopExecParser execParser(_service, {});

    return !QProcess::startDetached(KIO::DesktopExecParser::executableName(_service.exec()), execParser.resultingArguments());
}
