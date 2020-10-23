/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamUtils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <QDebug>
#include "utils.h"
#include <KLocalizedString>
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

    QString changelog = QLatin1String("<h3>") + release.version() + QLatin1String("</h3>")
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
    return {thumbnails, screenshots};
}

QJsonArray AppStreamUtils::licenses(const AppStream::Component& appdata)
{
#if APPSTREAM_HAS_SPDX
    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(appdata.projectLicense());
#if !APPSTREAM_HAS_SPDX_LICENSEURL
    static const QLatin1String prop ("@LicenseRef-proprietary=");
#endif
    for (const auto &token : licenses) {
        QString license = token;
        license.remove(0, 1); //tokenize prefixes with an @ for some reason
        if (!AppStream::SPDX::isLicenseId(license))
            continue;
#if APPSTREAM_HAS_SPDX_LICENSEURL
        ret.append(QJsonObject{ {QStringLiteral("name"), license}, {QStringLiteral("url"), { AppStream::SPDX::licenseUrl(license) } }});
#else
        if (license.startsWith(prop))
            ret.append(QJsonObject{ {QStringLiteral("name"), i18n("Proprietary")}, {QStringLiteral("url"), license.mid(prop.size())} });
        else
            ret.append(QJsonObject{ {QStringLiteral("name"), license}, {QStringLiteral("url"), { QLatin1String("https://spdx.org/licenses/") + AppStream::SPDX::asSpdxId(license) + QLatin1String(".html#licenseText") } }});
#endif
    }
    return ret;
#else
    return { QJsonObject { {QStringLiteral("name"), appdata.projectLicense() } } };
#endif
}

QStringList AppStreamUtils::appstreamIds(const QUrl &appstreamUrl)
{
    QStringList ret;
    ret += appstreamUrl.host().isEmpty() ? appstreamUrl.path() : appstreamUrl.host();
    if (appstreamUrl.hasQuery()) {
        QUrlQuery query(appstreamUrl);
        ret << query.queryItemValue(QStringLiteral("alt")).split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    return ret;
}
