/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <AppStreamQt/component.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/pool.h>
#include <QColor>
#include <QCoroTask>
#include <QList>
#include <QUrl>
#include <resources/AbstractResource.h>

#include "discovercommon_export.h"

namespace AppStreamUtils
{
DISCOVERCOMMON_EXPORT std::pair<QUrl, size_t> imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind);

DISCOVERCOMMON_EXPORT QString changelogToHtml(const AppStream::Component &appdata);

DISCOVERCOMMON_EXPORT Screenshots fetchScreenshots(const AppStream::Component &appdata);

DISCOVERCOMMON_EXPORT QJsonArray licenses(const AppStream::Component &appdata);

DISCOVERCOMMON_EXPORT QJsonArray licenses(const QString &spdxExpression);

DISCOVERCOMMON_EXPORT QJsonObject license(const QString &spdxId);

DISCOVERCOMMON_EXPORT QStringList appstreamIds(const QUrl &appstreamUrl);

/// Helps implement AbstractResource::versionString
DISCOVERCOMMON_EXPORT QString versionString(const QString &version, const AppStream::Component &appdata);

DISCOVERCOMMON_EXPORT QString contentRatingDescription(const AppStream::Component &appdata);
DISCOVERCOMMON_EXPORT uint contentRatingMinimumAge(const AppStream::Component &appdata);

DISCOVERCOMMON_EXPORT QCoro::Task<AppStream::ComponentBox> componentsByCategoriesTask(AppStream::Pool *pool, Category *cat, AppStream::Bundle::Kind kind);

DISCOVERCOMMON_EXPORT bool kIconLoaderHasIcon(const QString &name);
}
