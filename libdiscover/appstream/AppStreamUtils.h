/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef APPSTREAMUTILS_H
#define APPSTREAMUTILS_H

#include <QUrl>
#include <QList>
#include "libdiscover_debug.h"
#include <AppStreamQt/image.h>
#include <AppStreamQt/component.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>

namespace AppStreamUtils
{
static QUrl imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Image &i, images) {
        if (i.kind() == kind) {
            ret = i.url();
            break;
        }
    }
    return ret;
}

static QString changelogToHtml(const AppStream::Component &appdata)
{
    if(appdata.releases().isEmpty())
        return {};

    const auto release = appdata.releases().constFirst();
    if (release.description().isEmpty())
        return {};

    QString changelog = QStringLiteral("<h3>") + release.version() + QStringLiteral("</h3>")
                      + QStringLiteral("<p>") + release.description() + QStringLiteral("</p>");
    return changelog;
}

static QPair<QList<QUrl>, QList<QUrl>> fetchScreenshots(const AppStream::Component &appdata)
{
    QList<QUrl> screenshots, thumbnails;
    Q_FOREACH (const AppStream::Screenshot &s, appdata.screenshots()) {
        const auto images = s.images();
        const QUrl thumbnail = AppStreamUtils::imageOfKind(images, AppStream::Image::KindThumbnail);
        const QUrl plain = AppStreamUtils::imageOfKind(images, AppStream::Image::KindSource);
        if (plain.isEmpty())
            qCWarning(LIBDISCOVER_LOG) << "invalid screenshot for" << appdata.name();

        screenshots << plain;
        thumbnails << (thumbnail.isEmpty() ? plain : thumbnail);
    }
    return {screenshots, thumbnails};
}

}

#endif
