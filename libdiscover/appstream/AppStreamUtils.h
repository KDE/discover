/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef APPSTREAMUTILS_H
#define APPSTREAMUTILS_H

#include <AppStreamQt/component.h>
#include <AppStreamQt/image.h>
#include <QColor>
#include <QList>
#include <QUrl>
#include <resources/AbstractResource.h>

namespace AppStreamUtils
{
Q_DECL_EXPORT QUrl imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind);

Q_DECL_EXPORT QString changelogToHtml(const AppStream::Component &appdata);

Q_DECL_EXPORT QPair<QList<QUrl>, QList<QUrl>> fetchScreenshots(const AppStream::Component &appdata);

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
}

#endif
