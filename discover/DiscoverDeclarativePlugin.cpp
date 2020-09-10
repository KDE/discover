/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverDeclarativePlugin.h"
#include "ReadFile.h"
#include <Category/CategoryModel.h>
#include <Category/Category.h>
#include <Transaction/TransactionListener.h>
#include <Transaction/TransactionModel.h>
#include <Transaction/Transaction.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/SourcesModel.h>
#include <resources/AbstractSourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesProxyModel.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <UpdateModel/UpdateModel.h>
#include <ScreenshotsModel.h>
#include <ApplicationAddonsModel.h>
#include <ActionsModel.h>
#include <qqml.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QAction>

void DiscoverDeclarativePlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
    engine->rootContext()->setContextProperty(QStringLiteral("ResourcesModel"), ResourcesModel::global());
    engine->rootContext()->setContextProperty(QStringLiteral("TransactionModel"), TransactionModel::global());
    engine->rootContext()->setContextProperty(QStringLiteral("SourcesModel"), SourcesModel::global());
    engine->rootContext()->setContextProperty(QStringLiteral("CategoryModel"), CategoryModel::global());
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

void DiscoverDeclarativePlugin::registerTypes(const char* /*uri*/)
{
    qmlRegisterType<TransactionListener>("org.kde.discover", 2, 0, "TransactionListener");
    qmlRegisterType<ResourcesUpdatesModel>("org.kde.discover", 2, 0, "ResourcesUpdatesModel");
    qmlRegisterType<ResourcesProxyModel>("org.kde.discover", 2, 0, "ResourcesProxyModel");

    qmlRegisterType<ReviewsModel>("org.kde.discover", 2, 0, "ReviewsModel");
    qmlRegisterType<ApplicationAddonsModel>("org.kde.discover", 2, 0, "ApplicationAddonsModel");
    qmlRegisterType<ScreenshotsModel>("org.kde.discover", 2, 0, "ScreenshotsModel");
    qmlRegisterType<ActionsModel>("org.kde.discover", 2, 0, "ActionsModel");
    qmlRegisterType<UpdateModel>("org.kde.discover", 2, 0, "UpdateModel");
    qmlRegisterType<ReadFile>("org.kde.discover", 2, 0, "ReadFile");

    qmlRegisterUncreatableType<QAction>("org.kde.discover", 2, 0, "QAction", QStringLiteral("Use QQC Action"));
    qmlRegisterUncreatableType<AbstractResource>("org.kde.discover", 2, 0, "AbstractResource", QStringLiteral("should come from the ResourcesModel"));
    qmlRegisterUncreatableType<AbstractSourcesBackend>("org.kde.discover", 2, 0, "AbstractSourcesBackend", QStringLiteral("should come from the SourcesModel"));
    qmlRegisterUncreatableType<Transaction>("org.kde.discover", 2, 0, "Transaction", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<SourcesModel>("org.kde.discover", 2, 0, "SourcesModelClass", QStringLiteral("should come from the backends"));
    qmlRegisterUncreatableType<SourcesModel>("org.kde.discover", 2, 0, "AbstractBackendUpdater", QStringLiteral("should come from the backends"));
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    qmlRegisterType<TransactionModel>();
    qmlRegisterType<Rating>();
    qmlRegisterType<AbstractResourcesBackend>();
    qmlRegisterType<AbstractReviewsBackend>();
    qmlRegisterType<Category>();
    qmlRegisterType<ResourcesModel>();
#else
    qmlRegisterAnonymousType<TransactionModel>("org.kde.discover", 1);
    qmlRegisterAnonymousType<Rating>("org.kde.discover", 1);
    qmlRegisterAnonymousType<AbstractResourcesBackend>("org.kde.discover", 1);
    qmlRegisterAnonymousType<AbstractReviewsBackend>("org.kde.discover", 1);
    qmlRegisterAnonymousType<Category>("org.kde.discover", 1);
    qmlRegisterAnonymousType<ResourcesModel>("org.kde.discover", 1);
#endif
    qmlProtectModule("org.kde.discover", 2);
    qRegisterMetaType<QList<QAction*>>();
}
