/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractResourcesBackend.h"
#include "Category/Category.h"
#include "libdiscover_debug.h"
#include "utils.h"
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

QDebug operator<<(QDebug debug, const StreamResult &sss)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "StreamResult(" << sss.resource << ", " << sss.sortScore << ')';
    return debug;
}

ResultsStream::ResultsStream(const QString &objectName, const QVector<StreamResult> &resources)
    : ResultsStream(objectName)
{
    Q_ASSERT(!kContains(resources, [](const StreamResult &result) {
        return result.resource == nullptr;
    }));
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
        qCDebug(LIBDISCOVER_LOG) << "took really long to fetch" << this;
    });

    connect(this, &AbstractResourcesBackend::fetchingChanged, this, [this, fetchingChangedTimer] {
        // Q_ASSERT(isFetching() != fetchingChangedTimer->isActive());
        if (isFetching()) {
            fetchingChangedTimer->start();
        } else {
            fetchingChangedTimer->stop();
        }

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

bool AbstractResourcesBackend::Filters::shouldFilter(AbstractResource *resourse) const
{
    Q_ASSERT(resourse);

    if (!extends.isEmpty() && !resourse->extends().contains(extends)) {
        return false;
    }

    if (!origin.isEmpty() && resourse->origin() != origin) {
        return false;
    }

    if (filterMinimumState ? (resourse->state() < state) : (resourse->state() != state)) {
        return false;
    }

    if (!mimetype.isEmpty() && !resourse->mimetypes().contains(mimetype)) {
        return false;
    }

    return !category || resourse->categoryMatches(category);
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<AbstractResource *> &resources) const
{
    for (auto it = resources.begin(); it != resources.end();) {
        if (shouldFilter(*it)) {
            ++it;
        } else {
            it = resources.erase(it);
        }
    }
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<StreamResult> &results) const
{
    for (auto it = results.begin(); it != results.end();) {
        if (shouldFilter(it->resource)) {
            ++it;
        } else {
            it = results.erase(it);
        }
    }
}

bool AbstractResourcesBackend::extends(const QString & /*id*/) const
{
    return false;
}

int AbstractResourcesBackend::fetchingUpdatesProgress() const
{
    return isFetching() ? 42 : 100;
}

uint AbstractResourcesBackend::fetchingUpdatesProgressWeight() const
{
    return 1;
}

InlineMessage *AbstractResourcesBackend::explainDysfunction() const
{
    return new InlineMessage(InlineMessage::Error, QStringLiteral("network-disconnect"), i18n("Please verify Internet connectivity"));
}

#include "moc_AbstractResourcesBackend.cpp"
