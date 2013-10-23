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
#include "MuonExporter.h"

static const char description[] =
    I18N_NOOP("An application exporterer");

static const char version[] = "2.1.65";

int main(int argc, char** argv)
{
    KAboutData about("muon-exporter", "muon-exporter", ki18n("Muon Exporter"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("©2010-2012 Jonathan Thomas"), KLocalizedString(), 0);
    about.addAuthor(ki18n("Jonathan Thomas"), KLocalizedString(), "echidnaman@kubuntu.org");
    about.addAuthor(ki18n("Aleix Pol Gonzalez"), KLocalizedString(), "aleixpol@blue-systems.com");
    about.setProgramIconName("muonexporter");
    about.setProductName("muon/exporter");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineOptions options;
    options.add("+file", ki18n("File to which we'll export"));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    MuonExporter exp;
    exp.setExportPath(args->url(0));
    QObject::connect(&exp, SIGNAL(exportDone()), &app, SLOT(quit()));

    return app.exec();
}
