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

typedef QHash<QString, DiscoverMainWindow::CompactMode> StringCompactMode;
Q_GLOBAL_STATIC_WITH_ARGS(StringCompactMode, s_decodeCompactMode, (StringCompactMode {
    { QLatin1String("auto"), DiscoverMainWindow::Auto },
    { QLatin1String("compact"), DiscoverMainWindow::Compact },
    { QLatin1String("full"), DiscoverMainWindow::Full }
}))

QCommandLineParser* createParser()
{
    QCommandLineParser* parser = new QCommandLineParser;
    parser->addOption(QCommandLineOption(QStringLiteral("application"), i18n("Directly open the specified application by its package name."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("mime"), i18n("Open with a program that can deal with the given mimetype."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("category"), i18n("Display a list of entries with a category."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("mode"), i18n("Open Discover in a said mode. Modes correspond to the toolbar buttons."), QStringLiteral("name")));
    parser->addOption(QCommandLineOption(QStringLiteral("listmodes"), i18n("List all the available modes.")));
    parser->addOption(QCommandLineOption(QStringLiteral("compact"), i18n("Compact Mode (auto/compact/full)."), QStringLiteral("mode"), QStringLiteral("auto")));
    parser->addOption(QCommandLineOption(QStringLiteral("local-filename"), i18n("Local package file to install"), QStringLiteral("package")));
    parser->addOption(QCommandLineOption(QStringLiteral("listbackends"), i18n("List all the available backends.")));
    parser->addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Test file"), QStringLiteral("file.qml")));
    parser->addPositionalArgument(QStringLiteral("urls"), i18n("Supports appstream: url scheme"));
    DiscoverBackendsFactory::setupCommandLine(parser);
    KAboutData::applicationData().setupCommandLine(parser);
    parser->addHelpOption();
    parser->addVersionOption();
    return parser;
}

void processArgs(QCommandLineParser* parser, DiscoverMainWindow* mainWindow)
{
    if(parser->isSet(QStringLiteral("application")))
        mainWindow->openApplication(QUrl(parser->value(QStringLiteral("application"))));
    else if(parser->isSet(QStringLiteral("mime")))
        mainWindow->openMimeType(parser->value(QStringLiteral("mime")));
    else if(parser->isSet(QStringLiteral("category")))
        mainWindow->openCategory(parser->value(QStringLiteral("category")));

    if(parser->isSet(QStringLiteral("mode")))
        mainWindow->openMode(parser->value(QStringLiteral("mode")));

    if(parser->isSet(QStringLiteral("local-filename")))
        mainWindow->openLocalPackage(QUrl::fromUserInput(parser->value(QStringLiteral("local-filename")), {}, QUrl::AssumeLocalFile));

    foreach(const QString &arg, parser->positionalArguments()) {
        const QUrl url = QUrl::fromUserInput(arg, {}, QUrl::AssumeLocalFile);
        if (url.isLocalFile())
            mainWindow->openLocalPackage(url);
        else
            mainWindow->openApplication(url);
    }
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("plasmadiscover")));
    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    KCrash::initialize();
    KLocalizedString::setApplicationDomain("plasma-discover");
    KAboutData about(QStringLiteral("discover"), i18n("Discover"), version, i18n("An application explorer"),
                     KAboutLicense::GPL, i18n("© 2010-2016 Plasma Development Team"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@blue-systems.com"));
    about.addAuthor(i18n("Jonathan Thomas"), QString(), QStringLiteral("echidnaman@kubuntu.org"));
    about.setProductName("discover/discover");
    KAboutData::setApplicationData(about);

    DiscoverMainWindow *mainWindow = nullptr;
    {
        QScopedPointer<QCommandLineParser> parser(createParser());
        parser->process(app);
        about.processCommandLine(parser.data());
        DiscoverBackendsFactory::processCommandLine(parser.data(), parser->isSet(QStringLiteral("test")));

        if(parser->isSet(QStringLiteral("listbackends"))) {
            QTextStream(stdout) << i18n("Available backends:\n");
            DiscoverBackendsFactory f;
            foreach(const QString& name, f.allBackendNames(false, true))
                QTextStream(stdout) << " * " << name << '\n';
            return 0;
        }

        if (parser->isSet(QStringLiteral("test"))) {
            QStandardPaths::setTestModeEnabled(true);
        }

        KDBusService* service = new KDBusService(KDBusService::Unique, &app);

        mainWindow = new DiscoverMainWindow(s_decodeCompactMode->value(parser->value(QStringLiteral("compact")), DiscoverMainWindow::Full));
        QObject::connect(&app, &QCoreApplication::aboutToQuit, mainWindow, &DiscoverMainWindow::deleteLater);
        QObject::connect(service, &KDBusService::activateRequested, mainWindow, [mainWindow](const QStringList &arguments, const QString &/*workingDirectory*/){
            mainWindow->rootObject()->raise();
            if (arguments.isEmpty())
                return;
            QScopedPointer<QCommandLineParser> parser(createParser());
            parser->process(arguments);
            processArgs(parser.data(), mainWindow);
        });

        processArgs(parser.data(), mainWindow);

        if(parser->isSet(QStringLiteral("listmodes"))) {
            QTextStream(stdout) << i18n("Available modes:\n");
            foreach(const QString& mode, mainWindow->modes())
                QTextStream(stdout) << " * " << mode << '\n';
            delete mainWindow;
            return 0;
        }

        if (parser->isSet(QStringLiteral("test"))) {
            const QUrl testFile = QUrl::fromUserInput(parser->value(QStringLiteral("test")), {}, QUrl::AssumeLocalFile);
            Q_ASSERT(!testFile.isEmpty() && testFile.isLocalFile());

            mainWindow->loadTest(testFile);
        }
    }

    return app.exec();
}
