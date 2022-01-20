/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractResourcesBackend.h"
#include "Category/Category.h"
#include "libdiscover_debug.h"
#include <KLocalizedString>
#include <QHash>
#include <QMetaObject>
#include <QMetaProperty>
#include <QTimer>

QDebug operator<<(QDebug debug, const AbstractResourcesBackend::Filters &filters)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Filters(";
    if (filters.category)
        debug.nospace() << "category: " << filters.category << ',';
    if (filters.state)
        debug.nospace() << "state: " << filters.state << ',';
    if (!filters.mimetype.isEmpty())
        debug.nospace() << "mimetype: " << filters.mimetype << ',';
    if (!filters.search.isEmpty())
        debug.nospace() << "search: " << filters.search << ',';
    if (!filters.extends.isEmpty())
        debug.nospace() << "extends:" << filters.extends << ',';
    if (!filters.origin.isEmpty())
        debug.nospace() << "origin:" << filters.origin << ',';
    if (!filters.resourceUrl.isEmpty())
        debug.nospace() << "resourceUrl:" << filters.resourceUrl << ',';
    debug.nospace() << ')';

    return debug;
}

ResultsStream::ResultsStream(const QString &objectName, const QVector<AbstractResource *> &resources)
    : ResultsStream(objectName)
{
    Q_ASSERT(!resources.contains(nullptr));
    QTimer::singleShot(0, this, [resources, this]() {
        if (!resources.isEmpty())
            Q_EMIT resourcesFound(resources);
        finish();
    });
}

ResultsStream::ResultsStream(const QString &objectName)
{
    setObjectName(objectName);
    QTimer::singleShot(5000, this, [objectName]() {
        qCDebug(LIBDISCOVER_LOG) << "stream took really long" << objectName;
    });
}

ResultsStream::~ResultsStream()
{
}

void ResultsStream::finish()
{
    deleteLater();
}

AbstractResourcesBackend::AbstractResourcesBackend(QObject *parent)
    : QObject(parent)
{
    QTimer *fetchingChangedTimer = new QTimer(this);
    fetchingChangedTimer->setInterval(3000);
    fetchingChangedTimer->setSingleShot(true);
    connect(fetchingChangedTimer, &QTimer::timeout, this, [this] {
        qDebug() << "took really long to fetch" << this;
    });

    connect(this, &AbstractResourcesBackend::fetchingChanged, this, [this, fetchingChangedTimer] {
        // Q_ASSERT(isFetching() != fetchingChangedTimer->isActive());
        if (isFetching())
            fetchingChangedTimer->start();
        else
            fetchingChangedTimer->stop();

        Q_EMIT fetchingUpdatesProgressChanged();
    });
}

Transaction *AbstractResourcesBackend::installApplication(AbstractResource *app)
{
    return installApplication(app, AddonList());
}

void AbstractResourcesBackend::setName(const QString &name)
{
    m_name = name;
}

QString AbstractResourcesBackend::name() const
{
    return m_name;
}

void AbstractResourcesBackend::emitRatingsReady()
{
    Q_EMIT allDataChanged({"rating", "ratingPoints", "ratingCount", "sortableRating"});
}

bool AbstractResourcesBackend::Filters::shouldFilter(AbstractResource *res) const
{
    Q_ASSERT(res);

    if (!extends.isEmpty() && !res->extends().contains(extends)) {
        return false;
    }

    if (!origin.isEmpty() && res->origin() != origin) {
        return false;
    }

    if (filterMinimumState ? (res->state() < state) : (res->state() != state)) {
        return false;
    }

    if (!mimetype.isEmpty() && !res->mimetypes().contains(mimetype)) {
        return false;
    }

    if (resourceType.has_value() && res->type() != resourceType.value()) {
        return false;
    }

    return !category || res->categoryMatches(category);
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<AbstractResource *> &input) const
{
    for (auto it = input.begin(); it != input.end();) {
        if (shouldFilter(*it))
            ++it;
        else
            it = input.erase(it);
    }
}

QStringList AbstractResourcesBackend::extends() const
{
    return {};
}

int AbstractResourcesBackend::fetchingUpdatesProgress() const
{
    return isFetching() ? 42 : 100;
}

HelpfulError *AbstractResourcesBackend::explainDysfunction() const
{
    return new HelpfulError(QStringLiteral("network-disconnect"), i18n("Please verify Internet connectivity"));
}
