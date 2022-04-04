/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamUtils.h"

#include "utils.h"
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <KLocalizedString>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <AppStreamQt/spdx.h>

using namespace AppStreamUtils;

QUrl AppStreamUtils::imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
{
    QUrl ret;
    for (const AppStream::Image &i : images) {
        if (i.kind() == kind) {
            ret = i.url();
            break;
        }
    }
    return ret;
}

QString AppStreamUtils::changelogToHtml(const AppStream::Component &appdata)
{
    if (appdata.releases().isEmpty())
        return {};

    const auto release = appdata.releases().constFirst();
    if (release.description().isEmpty())
        return {};

    QString changelog =
        QLatin1String("<h3>") + release.version() + QLatin1String("</h3>") + QStringLiteral("<p>") + release.description() + QStringLiteral("</p>");
    return changelog;
}

QPair<QList<QUrl>, QList<QUrl>> AppStreamUtils::fetchScreenshots(const AppStream::Component &appdata)
{
    QList<QUrl> screenshots, thumbnails;
    const auto appdataScreenshots = appdata.screenshots();
    for (const AppStream::Screenshot &s : appdataScreenshots) {
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

QJsonArray AppStreamUtils::licenses(const AppStream::Component &appdata)
{
    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(appdata.projectLicense());
    for (const auto &token : licenses) {
        QString license = token;
        license.remove(0, 1); // tokenize prefixes with an @ for some reason

        bool publicLicense = false;
        QString name = license;
        if (license == QLatin1String("LicenseRef-proprietary")) {
            name = i18n("Proprietary");
        } else if (license == QLatin1String("LicenseRef-public-domain")) {
            name = i18n("Public Domain");
            publicLicense = true;
        }

        if (!AppStream::SPDX::isLicenseId(license))
            continue;
        ret.append(QJsonObject{
            {QStringLiteral("name"), name},
            {QStringLiteral("url"), {AppStream::SPDX::licenseUrl(license)}},
            {QStringLiteral("hasFreedom"), AppStream::SPDX::isFreeLicense(license) || publicLicense},
        });
    }
    return ret;
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

QString AppStreamUtils::versionString(const QString &version, const AppStream::Component &appdata)
{
    if (version.isEmpty()) {
        return {};
    } else {
        if (appdata.releases().isEmpty())
            return version;

        auto release = appdata.releases().constFirst();
        if (release.timestamp().isValid() && version.startsWith(release.version())) {
            QLocale l;
            return i18n("%1, released on %2", version, l.toString(release.timestamp().date(), QLocale::ShortFormat));
        } else {
            return version;
        }
    }
}
