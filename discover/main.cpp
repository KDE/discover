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

// #define QT_QML_DEBUG

#include <QApplication>
#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <qwindow.h>
#include "DiscoverMainWindow.h"
#include <DiscoverBackendsFactory.h>
#include "DiscoverVersion.h"
#include <QTextStream>
#include <QStandardPaths>

DiscoverMainWindow::CompactMode decodeCompactMode(const QString &str)
{
    if (str == QLatin1String("auto"))
        return DiscoverMainWindow::Auto;
    else if (str == QLatin1String("compact"))
        return DiscoverMainWindow::Compact;
    else if (str == QLatin1String("full"))
        return DiscoverMainWindow::Full;
    return DiscoverMainWindow::Full;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("muondiscover")));
    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#ifdef WITH_KCRASH_INIT
    KCrash::initialize();
#endif
    KLocalizedString::setApplicationDomain("plasma-discover");
    KAboutData about(QStringLiteral("muondiscover"), i18n("Discover"), version, i18n("An application explorer"),
                     KAboutLicense::GPL, i18n("© 2010-2016 Plasma Development Team"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@blue-systems.com"));
    about.addAuthor(i18n("Jonathan Thomas"), QString(), QStringLiteral("echidnaman@kubuntu.org"));
    about.setProductName("discover/discover");
    KAboutData::setApplicationData(about);

    KDBusService service(KDBusService::Unique);
    DiscoverMainWindow *mainWindow = nullptr;
    {
        QCommandLineParser parser;
        parser.addOption(QCommandLineOption(QStringLiteral("application"), i18n("Directly open the specified application by its package name."), QStringLiteral("name")));
        parser.addOption(QCommandLineOption(QStringLiteral("mime"), i18n("Open with a program that can deal with the given mimetype."), QStringLiteral("name")));
        parser.addOption(QCommandLineOption(QStringLiteral("category"), i18n("Display a list of entries with a category."), QStringLiteral("name")));
        parser.addOption(QCommandLineOption(QStringLiteral("mode"), i18n("Open Discover in a said mode. Modes correspond to the toolbar buttons."), QStringLiteral("name")));
        parser.addOption(QCommandLineOption(QStringLiteral("listmodes"), i18n("List all the available modes.")));
        parser.addOption(QCommandLineOption(QStringLiteral("compact"), i18n("Compact Mode (auto/compact/full)."), QStringLiteral("mode"), QStringLiteral("auto")));
        parser.addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Test file"), QStringLiteral("file.qml")));
        parser.addPositionalArgument(QStringLiteral("urls"), i18n("Supports appstream: url scheme"));
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.addHelpOption();
        parser.addVersionOption();
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser, parser.isSet(QStringLiteral("test")));

        if (parser.isSet(QStringLiteral("test"))) {
            QStandardPaths::setTestModeEnabled(true);
        }

        mainWindow = new DiscoverMainWindow(decodeCompactMode(parser.value(QStringLiteral("compact"))));
        QObject::connect(&app, &QApplication::aboutToQuit, mainWindow, &DiscoverMainWindow::deleteLater);

        if(parser.isSet(QStringLiteral("listmodes"))) {
            QTextStream(stdout) << i18n("Available modes:\n");
            foreach(const QString& mode, mainWindow->modes())
                QTextStream(stdout) << " * " << mode << '\n';
            return 0;
        }

        if(parser.isSet(QStringLiteral("application")))
            mainWindow->openApplication(parser.value(QStringLiteral("application")));
        else if(parser.isSet(QStringLiteral("mime")))
            mainWindow->openMimeType(parser.value(QStringLiteral("mime")));
        else if(parser.isSet(QStringLiteral("category")))
            mainWindow->openCategory(parser.value(QStringLiteral("category")));

        if(parser.isSet(QStringLiteral("mode")))
            mainWindow->openMode(parser.value(QStringLiteral("mode")).toLocal8Bit());

        foreach(const QString &arg, parser.positionalArguments()) {
            QUrl url(arg);
            if (url.scheme() == QLatin1String("appstream")) {
                mainWindow->openApplication(url.host());
            } else {
                QTextStream(stdout) << "unrecognized url" << url.toDisplayString() << '\n';
                return 1;
            }
        }

        if (parser.isSet(QStringLiteral("test"))) {
            const QUrl testFile = QUrl::fromUserInput(parser.value(QStringLiteral("test")), {}, QUrl::AssumeLocalFile);
            Q_ASSERT(!testFile.isEmpty() && testFile.isLocalFile());

            mainWindow->loadTest(testFile);
        }
    }

    return app.exec();
}
