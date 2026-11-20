// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BigscreenApp.h"
#include "DiscoverDeclarativePlugin.h"
#include "FeaturedModel.h"
#include "LimitedRowCountProxyModel.h"
#include "OdrsAppsModel.h"

#include <CachedNetworkAccessManager.h>

#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KWindowSystem>

#include <QApplication>
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

int runBigscreen(QApplication &app, KDBusService *service)
{
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

    if (service) {
        QObject::connect(service, &KDBusService::activateRequested, &engine, [&engine]() {
            if (auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
                window->show();
                KWindowSystem::updateStartupId(window);
                KWindowSystem::activateWindow(window);
            }
        });
    }

    return app.exec();
}
