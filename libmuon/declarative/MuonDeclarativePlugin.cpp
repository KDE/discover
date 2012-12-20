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

#include "MuonDeclarativePlugin.h"
#include "ApplicationProxyModelHelper.h"
#include <Category/CategoryModel.h>
#include <Category/Category.h>
#include <Transaction/TransactionModel.h>
#include <Transaction/Transaction.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <ScreenshotsModel.h>
#include <ApplicationAddonsModel.h>
#include <qdeclarative.h>

QML_DECLARE_TYPE(ResourcesModel)
QML_DECLARE_TYPE(AbstractResourcesBackend)

void MuonDeclarativePlugin::registerTypes(const char* uri)
{
    qmlRegisterType<CategoryModel>("org.kde.muon", 1, 0, "CategoryModel");
    //qmlRegisterType<TransactionModel>("org.kde.muon", 1, 0, "TransactionModel");
    qmlRegisterType<ResourcesUpdatesModel>("org.kde.muon", 1, 0, "ResourcesUpdatesModel");
    
    qmlRegisterType<ReviewsModel>("org.kde.muon", 1, 0, "ReviewsModel");
    qmlRegisterType<ApplicationAddonsModel>("org.kde.muon", 1, 0, "ApplicationAddonsModel");
    qmlRegisterType<ScreenshotsModel>("org.kde.muon", 1, 0, "ScreenshotsModel");
    qmlRegisterType<ApplicationProxyModelHelper>("org.kde.muon", 1, 0, "ApplicationProxyModel");
    
    qmlRegisterType<Rating>();
    qmlRegisterType<AbstractResource>();
    qmlRegisterType<AbstractResourcesBackend>();
    qmlRegisterType<AbstractReviewsBackend>();
    qmlRegisterType<Category>();
    qmlRegisterType<ResourcesModel>();
    qmlRegisterType<Transaction>();
}
