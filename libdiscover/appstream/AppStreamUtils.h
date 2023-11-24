/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#ifdef DISCOVER_USE_STABLE_APPSTREAM
#include <AppStreamQt5/component.h>
#include <AppStreamQt5/image.h>
#include <AppStreamQt5/pool.h>
#else
#include <AppStreamQt/component.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/pool.h>
#endif

#include <QColor>
#include <QList>
#include <QUrl>
#include <resources/AbstractResource.h>

namespace AppStreamUtils
{
Q_DECL_EXPORT QUrl imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind);

Q_DECL_EXPORT QString changelogToHtml(const AppStream::Component &appdata);

Q_DECL_EXPORT Screenshots fetchScreenshots(const AppStream::Component &appdata);

Q_DECL_EXPORT QJsonArray licenses(const AppStream::Component &appdata);

Q_DECL_EXPORT QJsonArray licenses(const QString &spdxExpression);

Q_DECL_EXPORT QJsonObject license(const QString &spdxId);

Q_DECL_EXPORT QStringList appstreamIds(const QUrl &appstreamUrl);

/// Helps implement AbstractResource::versionString
Q_DECL_EXPORT QString versionString(const QString &version, const AppStream::Component &appdata);

Q_DECL_EXPORT QString contentRatingText(const AppStream::Component &appdata);
Q_DECL_EXPORT QString contentRatingDescription(const AppStream::Component &appdata);
Q_DECL_EXPORT AbstractResource::ContentIntensity contentRatingIntensity(const AppStream::Component &appdata);
Q_DECL_EXPORT uint contentRatingMinimumAge(const AppStream::Component &appdata);

Q_DECL_EXPORT QList<AppStream::Component> componentsByCategories(AppStream::Pool *pool, Category *cat, AppStream::Bundle::Kind kind);
}
