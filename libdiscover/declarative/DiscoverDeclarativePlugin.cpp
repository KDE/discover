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

#include "DiscoverDeclarativePlugin.h"
#include "ApplicationProxyModelHelper.h"
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
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <UpdateModel/UpdateModel.h>
#include <ScreenshotsModel.h>
#include <ApplicationAddonsModel.h>
#include <MessageActionsModel.h>
#include <qqml.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QAction>

QML_DECLARE_TYPE(ResourcesModel)
QML_DECLARE_TYPE(AbstractResourcesBackend)

void DiscoverDeclarativePlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
    engine->rootContext()->setContextProperty(QStringLiteral("ResourcesModel"), ResourcesModel::global());
    engine->rootContext()->setContextProperty(QStringLiteral("TransactionModel"), TransactionModel::global());
    engine->rootContext()->setContextProperty(QStringLiteral("SourcesModel"), SourcesModel::global());
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

void DiscoverDeclarativePlugin::registerTypes(const char*)
{
    qmlRegisterType<CategoryModel>("org.kde.discover", 1, 0, "CategoryModel");
    qmlRegisterType<TransactionListener>("org.kde.discover", 1, 0, "TransactionListener");
    qmlRegisterType<TransactionModel>();
    qmlRegisterType<ResourcesUpdatesModel>("org.kde.discover", 1, 0, "ResourcesUpdatesModel");
    
    qmlRegisterType<ReviewsModel>("org.kde.discover", 1, 0, "ReviewsModel");
    qmlRegisterType<ApplicationAddonsModel>("org.kde.discover", 1, 0, "ApplicationAddonsModel");
    qmlRegisterType<ScreenshotsModel>("org.kde.discover", 1, 0, "ScreenshotsModel");
    qmlRegisterType<ApplicationProxyModelHelper>("org.kde.discover", 1, 0, "ApplicationProxyModel");
    qmlRegisterType<MessageActionsModel>("org.kde.discover", 1, 0, "MessageActionsModel");
    qmlRegisterType<UpdateModel>("org.kde.discover", 1, 0, "UpdateModel");
    
    qmlRegisterUncreatableType<QAction>("org.kde.discover", 1, 0, "QAction", QStringLiteral("Use QQC Action"));
    qmlRegisterType<Rating>();
    qmlRegisterType<AbstractResource>();
    qmlRegisterType<AbstractSourcesBackend>();
    qmlRegisterType<AbstractResourcesBackend>();
    qmlRegisterType<AbstractReviewsBackend>();
    qmlRegisterType<Category>();
    qmlRegisterType<ResourcesModel>();
    qmlRegisterType<Transaction>();
}
