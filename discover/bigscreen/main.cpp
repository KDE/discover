// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DiscoverDeclarativePlugin.h"
#include "DiscoverVersion.h"
#include "FeaturedModel.h"
#include "LimitedRowCountProxyModel.h"
#include "OdrsAppsModel.h"

#include <CachedNetworkAccessManager.h>
#include <DiscoverBackendsFactory.h>

#include <KAboutData>
#include <KConfig>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KWindowSystem>

#include <KirigamiAppDefaults>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QQuickWindow>

using namespace Qt::StringLiterals;

namespace
{
class CachedNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        return new CachedNetworkAccessManager(u"images"_s, parent);
    }
};
}

int main(int argc, char **argv)
{
    // needs to be set before we create the QGuiApplication
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager, true);

    KConfig::setMainConfigName(u"discoverrc"_s);

    QApplication app(argc, argv);
    KirigamiAppDefaults::apply(&app);

    app.setWindowIcon(QIcon::fromTheme(u"plasmadiscover"_s));
    app.setQuitLockEnabled(false);
    KLocalizedString::setApplicationDomain("plasma-discover");
    KAboutData about(u"discover-bigscreen"_s,
                     i18n("Discover"),
                     version,
                     i18n("An application explorer for Plasma Bigscreen"),
                     KAboutLicense::GPL,
                     i18n("© 2010-2026 Plasma Development Team"));
    about.addAuthor(i18n("Devin Lin"), QString(), u"devin@kde.org"_s);
    about.setProductName("discover/discover");
    about.setDesktopFileName(u"org.kde.discover.bigscreen"_s);
    about.setProgramLogo(app.windowIcon());
    about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(about);

    {
        QCommandLineParser parser;
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser, false);
    }

    KDBusService service(KDBusService::Unique);

    CachedNetworkAccessManagerFactory networkFactory;
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&networkFactory);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &engine, [&engine]() {
        qDeleteAll(engine.rootObjects());
    });
    KLocalization::setupLocalizedContext(&engine);

    const auto uriApp = "org.kde.discover.app";
    qmlRegisterType<LimitedRowCountProxyModel>(uriApp, 1, 0, "LimitedRowCountProxyModel");
    qmlRegisterType<FeaturedModel>(uriApp, 1, 0, "FeaturedModel");
    qmlRegisterType<OdrsAppsModel>(uriApp, 1, 0, "OdrsAppsModel");

    DiscoverDeclarativePlugin plugin;
    plugin.initializeEngine(&engine, "org.kde.discover");
    plugin.registerTypes("org.kde.discover");
    engine.loadFromModule("org.kde.discover.bigscreen", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    QObject::connect(&service, &KDBusService::activateRequested, &engine, [&engine]() {
        if (auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
            window->show();
            KWindowSystem::updateStartupId(window);
            KWindowSystem::activateWindow(window);
        }
    });

    return app.exec();
}
