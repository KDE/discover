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

static bool shouldFilter(AbstractResource *res, const QPair<FilterType, QString> &filter)
{
    bool ret = true;
    switch (filter.first) {
    case CategoryFilter:
        ret = res->categories().contains(filter.second);
        break;
    case PkgSectionFilter:
        ret = res->section() == filter.second;
        break;
    case PkgWildcardFilter: {
        QString wildcard = filter.second;
        wildcard.remove(QLatin1Char('*'));
        ret = res->packageName().contains(wildcard);
    } break;
    case AppstreamIdWildcardFilter: {
        QString wildcard = filter.second;
        wildcard.remove(QLatin1Char('*'));
        ret = res->appstreamId().contains(wildcard);
    } break;
    case PkgNameFilter: // Only useful in the not filters
        ret = res->packageName() == filter.second;
        break;
    case InvalidFilter:
        break;
    }
    return ret;
}

bool AbstractResource::categoryMatches(Category *cat)
{
    {
        const auto orFilters = cat->orFilters();
        bool orValue = orFilters.isEmpty();
        for (const auto &filter : orFilters) {
            if (shouldFilter(this, filter)) {
                orValue = true;
                break;
            }
        }
        if (!orValue)
            return false;
    }

    Q_FOREACH (const auto &filter, cat->andFilters()) {
        if (!shouldFilter(this, filter))
            return false;
    }

    Q_FOREACH (const auto &filter, cat->notFilters()) {
        if (shouldFilter(this, filter))
            return false;
    }
    return true;
}

static QSet<Category *> walkCategories(AbstractResource *res, const QVector<Category *> &cats)
{
    QSet<Category *> ret;
    foreach (Category *cat, cats) {
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

QString AbstractResource::categoryDisplay() const
{
    const auto matchedCategories = categoryObjects(CategoryModel::global()->rootCategories());
    QStringList ret;
    foreach (auto cat, matchedCategories) {
        ret.append(cat->name());
    }
    ret.sort();
    return ret.join(QLatin1String(", "));
}

QUrl AbstractResource::url() const
{
    const QString asid = appstreamId();
    return asid.isEmpty() ? QUrl(backend()->name() + QStringLiteral("://") + packageName()) : QUrl(QStringLiteral("appstream://") + asid);
}

QString AbstractResource::displayOrigin() const
{
    return i18nc("origin (backend name)", "%1 (%2)", origin(), backend()->displayName());
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
        return i18n("Update to version %1", available);
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
