/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include "libdiscover_debug.h"
#include <Category/CategoryModel.h>
#include <KFormat>
#include <KLocalizedString>
#include <KShell>
#include <QList>
#include <QProcess>
#include <QString>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>

AbstractResource::AbstractResource(AbstractResourcesBackend *parent)
    : QObject(parent)
{
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::sizeChanged);
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::versionsChanged);
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::reportNewState);
}

AbstractResource::~AbstractResource() = default;

QUrl AbstractResource::homepage()
{
    return QUrl();
}

QUrl AbstractResource::helpURL()
{
    return QUrl();
}

QUrl AbstractResource::bugURL()
{
    return QUrl();
}

QUrl AbstractResource::donationURL()
{
    return QUrl();
}

QUrl AbstractResource::contributeURL()
{
    return {};
}

void AbstractResource::addMetadata(const QString &key, const QJsonValue &value)
{
    m_metadata.insert(key, value);
}

QJsonValue AbstractResource::getMetadata(const QString &key)
{
    return m_metadata.value(key);
}

bool AbstractResource::canUpgrade()
{
    return state() == Upgradeable;
}

bool AbstractResource::isInstalled()
{
    return state() >= Installed;
}

void AbstractResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched({}, {});
}

QStringList AbstractResource::mimetypes() const
{
    return QStringList();
}

AbstractResourcesBackend *AbstractResource::backend() const
{
    return static_cast<AbstractResourcesBackend *>(parent());
}

QObject *AbstractResource::backendObject() const
{
    return parent();
}

QString AbstractResource::status()
{
    switch (state()) {
    case Broken:
        return i18n("Broken");
    case None:
        return i18n("Available");
    case Installed:
        return i18n("Installed");
    case Upgradeable:
        return i18n("Upgradeable");
    }
    return QString();
}

QString AbstractResource::sizeDescription()
{
    return KFormat().formatByteSize(size());
}

QCollatorSortKey AbstractResource::nameSortKey()
{
    if (!m_collatorKey) {
        m_collatorKey.reset(new QCollatorSortKey(QCollator().sortKey(name())));
    }
    return *m_collatorKey;
}

Rating *AbstractResource::rating() const
{
    AbstractReviewsBackend *ratings = backend()->reviewsBackend();
    return ratings ? ratings->ratingForApplication(const_cast<AbstractResource *>(this)) : nullptr;
}

QVariant AbstractResource::ratingVariant() const
{
    auto instance = rating();
    return instance ? QVariant::fromValue<Rating>(*instance) : QVariant();
}

QStringList AbstractResource::extends() const
{
    return {};
}

QString AbstractResource::appstreamId() const
{
    return {};
}

void AbstractResource::reportNewState()
{
    if (backend()->isFetching())
        return;

    static const QVector<QByteArray> ns = {"state", "status", "canUpgrade", "size", "sizeDescription", "installedVersion", "availableVersion"};
    Q_EMIT backend()->resourcesChanged(this, ns);
}

static bool shouldFilter(AbstractResource *res, const CategoryFilter &filter)
{
    bool ret = true;
    switch (filter.type) {
    case CategoryFilter::CategoryNameFilter:
        ret = res->categories().contains(std::get<QString>(filter.value));
        break;
    case CategoryFilter::PkgSectionFilter:
        ret = res->section() == std::get<QString>(filter.value);
        break;
    case CategoryFilter::PkgWildcardFilter: {
        QString wildcard = std::get<QString>(filter.value);
        wildcard.remove(QLatin1Char('*'));
        ret = res->packageName().contains(wildcard);
    } break;
    case CategoryFilter::AppstreamIdWildcardFilter: {
        QString wildcard = std::get<QString>(filter.value);
        wildcard.remove(QLatin1Char('*'));
        ret = res->appstreamId().contains(wildcard);
    } break;
    case CategoryFilter::PkgNameFilter: // Only useful in the not filters
        ret = res->packageName() == std::get<QString>(filter.value);
        break;
    case CategoryFilter::AndFilter: {
        const auto filters = std::get<QVector<CategoryFilter>>(filter.value);
        ret = std::all_of(filters.begin(), filters.end(), [res](const CategoryFilter &f) {
            return shouldFilter(res, f);
        });
        break;
    }
    case CategoryFilter::OrFilter: {
        const auto filters = std::get<QVector<CategoryFilter>>(filter.value);
        ret = std::any_of(filters.begin(), filters.end(), [res](const CategoryFilter &f) {
            return shouldFilter(res, f);
        });
        break;
    }
    case CategoryFilter::NotFilter: {
        const auto filters = std::get<QVector<CategoryFilter>>(filter.value);
        ret = !std::any_of(filters.begin(), filters.end(), [res](const CategoryFilter &f) {
            return shouldFilter(res, f);
        });
        break;
    }
    }
    return ret;
}

bool AbstractResource::categoryMatches(Category *cat)
{
    return shouldFilter(this, cat->filter());
}

static QSet<Category *> walkCategories(AbstractResource *res, const QVector<Category *> &cats)
{
    QSet<Category *> ret;
    for (Category *cat : cats) {
        if (res->categoryMatches(cat)) {
            const auto subcats = walkCategories(res, cat->subCategories());
            if (subcats.isEmpty()) {
                ret += cat;
            } else {
                ret += subcats;
            }
        }
    }

    return ret;
}

QSet<Category *> AbstractResource::categoryObjects(const QVector<Category *> &cats) const
{
    return walkCategories(const_cast<AbstractResource *>(this), cats);
}

QUrl AbstractResource::url() const
{
    const QString asid = appstreamId();
    return asid.isEmpty() ? QUrl(backend()->name() + QStringLiteral("://") + packageName()) : QUrl(QStringLiteral("appstream://") + asid);
}

QString AbstractResource::displayOrigin() const
{
    return origin();
}

QString AbstractResource::executeLabel() const
{
    return i18n("Launch");
}

QString AbstractResource::upgradeText() const
{
    QString installed = installedVersion(), available = availableVersion();
    if (installed == available) {
        // Update of the same version; show when old and new are
        // the same (common with Flatpak runtimes)
        return i18nc("@info 'Refresh' is used as a noun here, and %1 is an app's version number", "Refresh of version %1", available);
    } else if (!installed.isEmpty() && !available.isEmpty()) {
        // Old and new version numbers
        // This thing with \u009C is a fancy feature in QML text handling:
        // when the string will be elided, it shows the string after
        // the last \u009C. This allows us to show a smaller string
        // when there's now enough room

        // All of this is mostly for the benefit of KDE Neon users,
        // since the version strings there are really really long
        return i18nc("Do not translate or alter \\u009C", "%1 → %2\u009C%1 → %2\u009C%2", installed, available);
    } else {
        // Available version only, for when the installed version
        // isn't available for some reason
        return available;
    }
}

QString AbstractResource::versionString()
{
    const QString version = isInstalled() ? installedVersion() : availableVersion();
    if (version.isEmpty()) {
        return {};
    } else {
        QLocale l;
        const QString releaseString = l.toString(releaseDate(), QLocale::ShortFormat);
        if (!releaseString.isEmpty()) {
            return i18n("%1, released on %2", version, releaseString);
        } else {
            return version;
        }
    }
}

QString AbstractResource::contentRatingDescription() const
{
    return {};
}

AbstractResource::ContentIntensity AbstractResource::contentRatingIntensity() const
{
    return Mild;
}

QString AbstractResource::contentRatingText() const
{
    return {};
}

uint AbstractResource::contentRatingMinimumAge() const
{
    return 0;
}
