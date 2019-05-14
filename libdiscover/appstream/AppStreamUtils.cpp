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

#include "AppStreamUtils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include "utils.h"
#include <KLocalizedString>
#include <AppStreamQt/component.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#if APPSTREAM_HAS_SPDX
#include <AppStreamQt/spdx.h>
#endif

using namespace AppStreamUtils;

QUrl AppStreamUtils::imageOfKind(const QList<AppStream::Image>& images, AppStream::Image::Kind kind)
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

QString AppStreamUtils::changelogToHtml(const AppStream::Component& appdata)
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

QPair<QList<QUrl>, QList<QUrl> > AppStreamUtils::fetchScreenshots(const AppStream::Component& appdata)
{
    QList<QUrl> screenshots, thumbnails;
    Q_FOREACH (const AppStream::Screenshot &s, appdata.screenshots()) {
        const auto images = s.images();
        const QUrl thumbnail = AppStreamUtils::imageOfKind(images, AppStream::Image::KindThumbnail);
        const QUrl plain = AppStreamUtils::imageOfKind(images, AppStream::Image::KindSource);
        if (plain.isEmpty())
            qWarning() << "invalid screenshot for" << appdata.name();

        screenshots << plain;
        thumbnails << (thumbnail.isEmpty() ? plain : thumbnail);
    }
    return {screenshots, thumbnails};
}

QJsonArray AppStreamUtils::licenses(const AppStream::Component& appdata)
{
#if APPSTREAM_HAS_SPDX
    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(appdata.projectLicense());
    static const QLatin1String prop ("@LicenseRef-proprietary=");
    for (const auto &token : licenses) {
        QString license = token;
        license.remove(0, 1); //tokenize prefixes with an @ for some reason
        if (!AppStream::SPDX::isLicenseId(license))
            continue;

        if (license.startsWith(prop))
            ret.append(QJsonObject{ {QStringLiteral("name"), i18n("Proprietary")}, {QStringLiteral("url"), license.mid(prop.size())} });
        else
            ret.append(QJsonObject{ {QStringLiteral("name"), license}, {QStringLiteral("url"), { QLatin1String("https://spdx.org/licenses/") + AppStream::SPDX::asSpdxId(license) + QLatin1String(".html#licenseText") } }});
    }
    return ret;
#else
    return { QJsonObject { {QStringLiteral("name"), appdata.projectLicense() } } };
#endif
}
