/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamUtils.h"

#include "utils.h"
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/spdx.h>
#include <AppStreamQt/version.h>
#include <KLocalizedString>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

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
    return licenses(appdata.projectLicense());
}

QJsonArray AppStreamUtils::licenses(const QString &spdx)
{
    static const QSet<QChar> tokens = {'&', '+', '|', '^', '(', ')'};

    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(spdx);
    for (const auto &token : licenses) {
        if (token.size() == 1 && tokens.contains(token.at(0)))
            continue;
        ret += license(token.mid(1)); // tokenize prefixes with an @ for some reason
    }
    return ret;
}

QJsonObject AppStreamUtils::license(const QString &license)
{
    bool publicLicense = false;
    QString name = license;
    if (license.startsWith(QLatin1String("LicenseRef-proprietary"))) {
        name = i18n("Proprietary");
    } else if (license == QLatin1String("LicenseRef-public-domain")) {
        name = i18n("Public Domain");
        publicLicense = true;
    }

    if (license.isEmpty()) {
        return {
            {QStringLiteral("name"), i18n("Unknown")},
            {QStringLiteral("hasFreedom"), true}, // give it the benefit of the doubt
        };
    }
    if (!AppStream::SPDX::isLicenseId(license))
        return {
            {QStringLiteral("name"), name},
            {QStringLiteral("hasFreedom"), true}, // give it the benefit of the doubt
        };
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("url"), {AppStream::SPDX::licenseUrl(license)}},
        {QStringLiteral("hasFreedom"), AppStream::SPDX::isFreeLicense(license) || publicLicense},
    };
}

QStringList AppStreamUtils::appstreamIds(const QUrl &appstreamUrl)
{
    QStringList ret;
    ret += appstreamUrl.host().isEmpty() ? appstreamUrl.path() : appstreamUrl.host();
    if (appstreamUrl.hasQuery()) {
        QUrlQuery query(appstreamUrl);
        ret << query.queryItemValue(QStringLiteral("alt")).split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (ret.removeDuplicates() != 0) {
        qDebug() << "received malformed url" << appstreamUrl;
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

QString AppStreamUtils::contentRatingDescription(const AppStream::Component &appdata)
{
#if ASQ_CHECK_VERSION(0, 15, 6)
    const auto ratings = appdata.contentRatings();
    QString ret;
    for (const auto &r : ratings) {
        const auto ratingIds = r.ratingIds();
        for (const auto &id : ratingIds) {
            if (r.value(id) != AppStream::ContentRating::RatingValueNone) {
                ret += QLatin1String("* ") + r.description(id) + QLatin1Char('\n');
            }
        }
    }

    return ret;
#else
    Q_UNUSED(appdata);
    return {};
#endif
}

QString AppStreamUtils::contentRatingText(const AppStream::Component &appdata)
{
#if ASQ_CHECK_VERSION(0, 15, 6)
    const auto ratings = appdata.contentRatings();
    AppStream::ContentRating::RatingValue intensity = AppStream::ContentRating::RatingValueUnknown;
    for (const auto &r : ratings) {
        const auto ratingIds = r.ratingIds();
        for (const auto &id : ratingIds) {
            intensity = std::max(r.value(id), intensity);
        }
    }

    static QStringList texts = {
        {},
        i18n("All Audiences"),
        i18nc("As specified in OARS, intensity of contents", "Mild Content"),
        i18nc("As specified in OARS, intensity of contents", "Moderate Content"),
        i18nc("As specified in OARS, intensity of contents", "Intense Content"),
    };
    return texts[intensity];
#else
    Q_UNUSED(appdata);
    return {};
#endif
}

AbstractResource::ContentIntensity AppStreamUtils::contentRatingIntensity(const AppStream::Component &appdata)
{
#if ASQ_CHECK_VERSION(0, 15, 6)
    const auto ratings = appdata.contentRatings();
    AppStream::ContentRating::RatingValue intensity = AppStream::ContentRating::RatingValueUnknown;
    for (const auto &r : ratings) {
        const auto ratingIds = r.ratingIds();
        for (const auto &id : ratingIds) {
            intensity = std::max(r.value(id), intensity);
        }
    }

    static QVector<AbstractResource::ContentIntensity> intensities = {
        AbstractResource::Mild,
        AbstractResource::Mild,
        AbstractResource::Mild,
        AbstractResource::Intense,
        AbstractResource::Intense,
    };
    return intensities[intensity];
#else
    Q_UNUSED(appdata);
    return {};
#endif
}

uint AppStreamUtils::contentRatingMinimumAge(const AppStream::Component &appdata)
{
#if ASQ_CHECK_VERSION(0, 15, 6)
    const auto ratings = appdata.contentRatings();
    uint minimumAge = 0;
    for (const auto &r : ratings) {
        minimumAge = std::max(r.minimumAge(), minimumAge);
    }
    return minimumAge;
#else
    Q_UNUSED(appdata);
    return 0;
#endif
}
