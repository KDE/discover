/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamUtils.h"

#include "utils.h"
#include <AppStreamQt/pool.h>
#include <AppStreamQt/release.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/spdx.h>
#include <AppStreamQt/version.h>
#include <Category/Category.h>
#include <KIconLoader>
#include <KLocalizedString>
#include <QCoroTimer>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

using namespace std::chrono_literals;
using namespace AppStreamUtils;

static size_t preferredScreenshotHeight()
{
    static size_t height = qGuiApp->devicePixelRatio() * 600;
    return height;
}

std::pair<QUrl, size_t> AppStreamUtils::imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
{
    QUrl ret;
    size_t height = -1;
    for (const AppStream::Image &i : images) {
        if (i.kind() == kind && (ret.isEmpty() || i.height() > height)) {
            ret = i.url();
            height = i.height();
            if (kind == AppStream::Image::KindSource || height > preferredScreenshotHeight()) {
                // there is only one source image and we don't need bigger screenshot than
                // preferredScreenshotHeight
                break;
            }
        }
    }
    return {ret, height};
}

QString AppStreamUtils::changelogToHtml(const AppStream::Component &appdata)
{
    const auto releases = appdata.releasesPlain();
    if (releases.isEmpty()) {
        return {};
    }

    const auto release = releases.indexSafe(0).value();
    if (release.description().isEmpty())
        return {};

    return QLatin1String("<h3>") + i18nc("@title:group", "Changes in version %1", release.version()) + QLatin1String("</h3>") + release.description();
}

Screenshots AppStreamUtils::fetchScreenshots(const AppStream::Component &appdata)
{
    const auto appdataScreenshots = appdata.screenshotsAll();
    Screenshots ret;
    ret.reserve(appdataScreenshots.size());
    for (const AppStream::Screenshot &s : appdataScreenshots) {
        const auto images = s.images();
        const auto [thumbnail, thumbnailHeight] = AppStreamUtils::imageOfKind(images, AppStream::Image::KindThumbnail);
        const auto [full, fullHeight] = AppStreamUtils::imageOfKind(images, AppStream::Image::KindSource);
        if (full.isEmpty()) {
            qWarning() << "AppStreamUtils: Invalid screenshot for" << appdata.name();
        }
        const bool isAnimated = s.mediaKind() == AppStream::Screenshot::MediaKindVideo;

        ret.append(Screenshot{thumbnail.isEmpty() || thumbnailHeight < preferredScreenshotHeight() ? full : thumbnail, full, isAnimated});
    }
    return ret;
}

QJsonArray AppStreamUtils::licenses(const AppStream::Component &appdata)
{
    return licenses(appdata.projectLicense());
}

QJsonArray AppStreamUtils::licenses(const QString &spdx)
{
    static const QSet<QChar> tokens = {QLatin1Char('&'), QLatin1Char('+'), QLatin1Char('|'), QLatin1Char('^'), QLatin1Char('('), QLatin1Char(')')};

    QJsonArray ret;
    const auto licenses = AppStream::SPDX::tokenizeLicense(spdx);
    for (const auto &token : licenses) {
        if (token.size() == 1 && tokens.contains(token.at(0)))
            continue;

        // Sometimes tokenize prefixes with an @ for some reason
        if (token.startsWith(QStringLiteral("@"))) {
            ret += license(token.mid(1));
        } else {
            ret += license(token);
        }
    }
    return ret;
}

QJsonObject AppStreamUtils::license(const QString &license)
{
    QString name = license;
    QString url = QString();
    QString licenseType = QStringLiteral("unknown");

    if (license.isEmpty() || license.compare(QStringLiteral("unknown"), Qt::CaseInsensitive) == 0) {
        name = i18n("Unknown");
    } else if (license.startsWith(QLatin1String("LicenseRef-proprietary"), Qt::CaseInsensitive)) {
        name = i18n("Proprietary");
        licenseType = QStringLiteral("proprietary");
    } else if (license.startsWith(QLatin1String("LicenseRef-public-domain"), Qt::CaseInsensitive)
               || license.contains(QLatin1String("public domain"), Qt::CaseInsensitive)) {
        name = i18n("Public Domain");
        licenseType = QStringLiteral("free");
    } else if (!AppStream::SPDX::isLicenseId(license)) {
        licenseType = QStringLiteral("non-free");
    } else {
        url = AppStream::SPDX::licenseUrl(license);
        licenseType = AppStream::SPDX::isFreeLicense(license) ? QStringLiteral("free") : QStringLiteral("non-free");
    }

    return {
        {QLatin1StringView("name"), name},
        {QLatin1StringView("url"), {url}},
        {QLatin1StringView("licenseType"), licenseType},
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
        qDebug() << "AppStreamUtils: Received malformed url" << appstreamUrl;
    }
    return ret;
}

QString AppStreamUtils::versionString(const QString &version, const AppStream::Component &appdata)
{
    Q_UNUSED(appdata);

    if (version.isEmpty()) {
        return {};
    }

    return version;
}

QString AppStreamUtils::contentRatingDescription(const AppStream::Component &appdata)
{
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
}

uint AppStreamUtils::contentRatingMinimumAge(const AppStream::Component &appdata)
{
    const auto ratings = appdata.contentRatings();
    uint minimumAge = 0;
    for (const auto &r : ratings) {
        minimumAge = std::max(r.minimumAge(), minimumAge);
    }
    return minimumAge;
}

static void kRemoveDuplicates(AppStream::ComponentBox &input, AppStream::Bundle::Kind kind)
{
    QSet<QString> ret;
    for (auto it = input.begin(); it != input.end();) {
        const auto key = kind == AppStream::Bundle::KindUnknown ? it->id() : it->bundle(kind).id();
        if (!ret.contains(key)) {
            ret << key;
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

QCoro::Task<AppStream::ComponentBox> AppStreamUtils::componentsByCategoriesTask(AppStream::Pool *pool, Category *cat, AppStream::Bundle::Kind kind)
{
    if (cat->name() == QLatin1StringView("All Applications")) {
        co_return pool->componentsByKind(AppStream::Component::KindDesktopApp);
    }

    AppStream::ComponentBox ret(AppStream::ComponentBox::FlagNoChecks);
    for (const auto &categoryName : cat->involvedCategories()) {
        // Give the eventloop some breathing room by suspending execution for a bit. This in particular should keep the
        // UI more responsive while we fetch a substantial amount of components on e.g. the all apps view.
        constexpr auto arbitrarySuspendTime = 64ms;
        QTimer timer;
        timer.start(arbitrarySuspendTime);
        co_await timer;

        ret += pool->componentsByCategories({categoryName});
    }
    kRemoveDuplicates(ret, kind);
    co_return ret;
}

DISCOVERCOMMON_EXPORT bool AppStreamUtils::kIconLoaderHasIcon(const QString &name)
{
    static auto icons = [] {
        auto icons = KIconLoader::global()->queryIcons(-1);
        return QSet<QString>(icons.cbegin(), icons.cend());
    }();
    return icons.contains(name);
}
