/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ResourcesModel.h"

#include "AbstractResource.h"
#include "Category/CategoryModel.h"
#include "Transaction/TransactionModel.h"
#include "libdiscover_debug.h"
#include "resources/AbstractBackendUpdater.h"
#include "resources/AbstractResourcesBackend.h"
#include "utils.h"
#include <DiscoverBackendsFactory.h>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KOSRelease>
#include <KSharedConfig>
#include <QCoreApplication>
#include <QIcon>
#include <QMetaProperty>
#include <QThread>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>
#include <Transaction/Transaction.h>
#include <functional>
#include <resources/DiscoverAction.h>

ResourcesModel *ResourcesModel::s_self = nullptr;

ResourcesModel *ResourcesModel::global()
{
    if (!s_self)
        s_self = new ResourcesModel;
    return s_self;
}

ResourcesModel::ResourcesModel(QObject *parent, bool load)
    : QObject(parent)
    , m_isFetching(false)
    , m_initializingBackends(0)
    , m_currentApplicationBackend(nullptr)
    , m_allInitializedEmitter(new QTimer(this))
    , m_updatesCount(
          0,
          [this] {
              {
                  int ret = 0;
                  for (AbstractResourcesBackend *backend : qAsConst(m_backends)) {
                      ret += backend->updatesCount();
                  }
                  return ret;
              }
          },
          [this](int count) {
              Q_EMIT updatesCountChanged(count);
          })
    , m_fetchingUpdatesProgress(
          0,
          [this] {
              {
                  if (m_backends.isEmpty())
                      return 0;

                  int sum = 0;
                  for (auto backend : qAsConst(m_backends)) {
                      sum += backend->fetchingUpdatesProgress();
                  }
                  return sum / m_backends.count();
              }
          },
          [this](int progress) {
              Q_EMIT fetchingUpdatesProgressChanged(progress);
          })
{
    init(load);
    connect(this, &ResourcesModel::allInitialized, this, &ResourcesModel::slotFetching);
    connect(this, &ResourcesModel::backendsChanged, this, &ResourcesModel::initApplicationsBackend);
}

void ResourcesModel::init(bool load)
{
    Q_ASSERT(!s_self);
    Q_ASSERT(QCoreApplication::instance()->thread() == QThread::currentThread());

    m_allInitializedEmitter->setSingleShot(true);
    m_allInitializedEmitter->setInterval(0);
    connect(m_allInitializedEmitter, &QTimer::timeout, this, [this]() {
        if (m_initializingBackends == 0)
            Q_EMIT allInitialized();
    });

    if (load)
        QMetaObject::invokeMethod(this, "registerAllBackends", Qt::QueuedConnection);

    m_updateAction = new DiscoverAction(this);
    m_updateAction->setIconName(QStringLiteral("system-software-update"));
    m_updateAction->setText(i18n("Refresh"));
    connect(this, &ResourcesModel::fetchingChanged, m_updateAction, [this](bool fetching) {
        m_updateAction->setEnabled(!fetching);
        m_fetchingUpdatesProgress.reevaluate();
    });
    connect(m_updateAction, &DiscoverAction::triggered, this, &ResourcesModel::checkForUpdates);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
}

ResourcesModel::ResourcesModel(const QString &backendName, QObject *parent)
    : ResourcesModel(parent, false)
{
    s_self = this;
    registerBackendByName(backendName);
}

ResourcesModel::~ResourcesModel()
{
    s_self = nullptr;
    qDeleteAll(m_backends);
}

void ResourcesModel::addResourcesBackend(AbstractResourcesBackend *backend)
{
    Q_ASSERT(!m_backends.contains(backend));
    if (!backend->isValid()) {
        qCWarning(LIBDISCOVER_LOG) << "Discarding invalid backend" << backend->name();
        CategoryModel::global()->blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }

    m_backends += backend;
    if (!backend->isFetching()) {
        m_updatesCount.reevaluate();
    } else {
        m_initializingBackends++;
    }

    connect(backend, &AbstractResourcesBackend::fetchingChanged, this, &ResourcesModel::callerFetchingChanged);
    connect(backend, &AbstractResourcesBackend::allDataChanged, this, &ResourcesModel::updateCaller);
    connect(backend, &AbstractResourcesBackend::resourcesChanged, this, &ResourcesModel::resourceDataChanged);
    connect(backend, &AbstractResourcesBackend::updatesCountChanged, this, [this] {
        m_updatesCount.reevaluate();
    });
    connect(backend, &AbstractResourcesBackend::fetchingUpdatesProgressChanged, this, [this] {
        m_fetchingUpdatesProgress.reevaluate();
    });
    connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &ResourcesModel::resourceRemoved);
    connect(backend, &AbstractResourcesBackend::passiveMessage, this, &ResourcesModel::passiveMessage);
    connect(backend->backendUpdater(), &AbstractBackendUpdater::progressingChanged, this, &ResourcesModel::slotFetching);
    if (backend->reviewsBackend()) {
        connect(backend->reviewsBackend(), &AbstractReviewsBackend::error, this, &ResourcesModel::passiveMessage, Qt::UniqueConnection);
    }

    // In case this is in fact the first backend to be added, and also happens to be
    // pre-filled, we still need for the rest of the backends to be added before trying
    // to send out the initialized signal. To ensure this happens, schedule it for the
    // start of the next run of the event loop.
    if (m_initializingBackends == 0) {
        m_allInitializedEmitter->start();
    } else {
        slotFetching();
    }
}

void ResourcesModel::callerFetchingChanged()
{
    AbstractResourcesBackend *backend = qobject_cast<AbstractResourcesBackend *>(sender());

    if (!backend->isValid()) {
        qCWarning(LIBDISCOVER_LOG) << "Discarding invalid backend" << backend->name();
        int idx = m_backends.indexOf(backend);
        Q_ASSERT(idx >= 0);
        m_backends.removeAt(idx);
        Q_EMIT backendsChanged();
        CategoryModel::global()->blacklistPlugin(backend->name());
        backend->deleteLater();
        slotFetching();
        return;
    }

    if (backend->isFetching()) {
        m_initializingBackends++;
        slotFetching();
    } else {
        m_initializingBackends--;
        if (m_initializingBackends == 0)
            m_allInitializedEmitter->start();
        else
            slotFetching();
    }
}

void ResourcesModel::updateCaller(const QVector<QByteArray> &properties)
{
    AbstractResourcesBackend *backend = qobject_cast<AbstractResourcesBackend *>(sender());

    Q_EMIT backendDataChanged(backend, properties);
}

QVector<AbstractResourcesBackend *> ResourcesModel::backends() const
{
    return m_backends;
}

bool ResourcesModel::hasSecurityUpdates() const
{
    bool ret = false;

    for (AbstractResourcesBackend *backend : qAsConst(m_backends)) {
        ret |= backend->hasSecurityUpdates();
    }

    return ret;
}

void ResourcesModel::installApplication(AbstractResource *app)
{
    TransactionModel::global()->addTransaction(app->backend()->installApplication(app));
}

void ResourcesModel::installApplication(AbstractResource *app, const AddonList &addons)
{
    TransactionModel::global()->addTransaction(app->backend()->installApplication(app, addons));
}

void ResourcesModel::removeApplication(AbstractResource *app)
{
    TransactionModel::global()->addTransaction(app->backend()->removeApplication(app));
}

void ResourcesModel::registerAllBackends()
{
    DiscoverBackendsFactory f;
    const auto backends = f.allBackends();
    if (m_initializingBackends == 0 && backends.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "Couldn't find any backends";
        m_allInitializedEmitter->start();
    } else {
        for (AbstractResourcesBackend *b : backends) {
            addResourcesBackend(b);
        }
        Q_EMIT backendsChanged();
    }
}

void ResourcesModel::registerBackendByName(const QString &name)
{
    DiscoverBackendsFactory f;
    const auto backends = f.backend(name);
    for (auto b : backends)
        addResourcesBackend(b);

    Q_EMIT backendsChanged();
}

bool ResourcesModel::isFetching() const
{
    return m_isFetching;
}

void ResourcesModel::slotFetching()
{
    bool newFetching = false;
    for (AbstractResourcesBackend *b : qAsConst(m_backends)) {
        // isFetching should sort of be enough. However, sometimes the backend itself
        // will still be operating on things, which from a model point of view would
        // still mean something going on. So, interpret that as fetching as well, for
        // the purposes of this property.
        if (b->isFetching() || (b->backendUpdater() && b->backendUpdater()->isProgressing())) {
            newFetching = true;
            break;
        }
    }
    if (newFetching != m_isFetching) {
        m_isFetching = newFetching;
        Q_EMIT fetchingChanged(m_isFetching);
    }
}

bool ResourcesModel::isBusy() const
{
    return TransactionModel::global()->rowCount() > 0;
}

bool ResourcesModel::isExtended(const QString &id)
{
    bool ret = true;
    for (AbstractResourcesBackend *backend : qAsConst(m_backends)) {
        ret = backend->extends().contains(id);
        if (ret)
            break;
    }
    return ret;
}

AggregatedResultsStream::AggregatedResultsStream(const QSet<ResultsStream *> &streams)
    : ResultsStream(QStringLiteral("AggregatedResultsStream"))
{
    Q_ASSERT(!streams.contains(nullptr));
    if (streams.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "no streams to aggregate!!";
        QTimer::singleShot(0, this, &AggregatedResultsStream::clear);
    }

    for (auto stream : streams) {
        connect(stream, &ResultsStream::resourcesFound, this, &AggregatedResultsStream::addResults);
        connect(stream, &QObject::destroyed, this, &AggregatedResultsStream::streamDestruction);
        connect(this, &ResultsStream::fetchMore, stream, &ResultsStream::fetchMore);
        m_streams << stream;
    }

    m_delayedEmission.setInterval(0);
    connect(&m_delayedEmission, &QTimer::timeout, this, &AggregatedResultsStream::emitResults);
}

AggregatedResultsStream::~AggregatedResultsStream() = default;

void AggregatedResultsStream::addResults(const QVector<AbstractResource *> &res)
{
    for (auto r : res)
        connect(r, &QObject::destroyed, this, &AggregatedResultsStream::resourceDestruction);

    m_results += res;

    m_delayedEmission.start();
}

void AggregatedResultsStream::emitResults()
{
    if (!m_results.isEmpty()) {
        Q_EMIT resourcesFound(m_results);
        m_results.clear();
    }
    m_delayedEmission.setInterval(m_delayedEmission.interval() + 100);
    m_delayedEmission.stop();
}

void AggregatedResultsStream::resourceDestruction(QObject *obj)
{
    m_results.removeAll(qobject_cast<AbstractResource *>(obj));
}

void AggregatedResultsStream::streamDestruction(QObject *obj)
{
    m_streams.remove(obj);
    clear();
}

void AggregatedResultsStream::clear()
{
    if (m_streams.isEmpty()) {
        emitResults();
        Q_EMIT finished();
        deleteLater();
    }
}

AggregatedResultsStream *ResourcesModel::search(const AbstractResourcesBackend::Filters &search)
{
    if (search.isEmpty()) {
        return new AggregatedResultsStream({new ResultsStream(QStringLiteral("emptysearch"), {})});
    }

    auto streams = kTransform<QSet<ResultsStream *>>(m_backends, [search](AbstractResourcesBackend *backend) {
        return backend->search(search);
    });
    return new AggregatedResultsStream(streams);
}

void ResourcesModel::checkForUpdates()
{
    for (auto backend : qAsConst(m_backends))
        backend->checkForUpdates();
}

AbstractResourcesBackend *ResourcesModel::currentApplicationBackend() const
{
    return m_currentApplicationBackend;
}

void ResourcesModel::setCurrentApplicationBackend(AbstractResourcesBackend *backend, bool write)
{
    if (backend != m_currentApplicationBackend) {
        if (write) {
            KConfigGroup settings(KSharedConfig::openConfig(), "ResourcesModel");
            if (backend)
                settings.writeEntry("currentApplicationBackend", backend->name());
            else
                settings.deleteEntry("currentApplicationBackend");
        }

        qCDebug(LIBDISCOVER_LOG) << "setting currentApplicationBackend" << backend;
        m_currentApplicationBackend = backend;
        Q_EMIT currentApplicationBackendChanged(backend);
    }
}

void ResourcesModel::initApplicationsBackend()
{
    const auto name = applicationSourceName();

    auto idx = kIndexOf(m_backends, [name](AbstractResourcesBackend *b) {
        return b->hasApplications() && b->name() == name;
    });
    if (idx < 0) {
        idx = kIndexOf(m_backends, [](AbstractResourcesBackend *b) {
            return b->hasApplications();
        });
        qCDebug(LIBDISCOVER_LOG) << "falling back applications backend to" << idx;
    }
    setCurrentApplicationBackend(m_backends.value(idx, nullptr), false);
}

QString ResourcesModel::applicationSourceName() const
{
    KConfigGroup settings(KSharedConfig::openConfig(), "ResourcesModel");
    return settings.readEntry<QString>("currentApplicationBackend", QStringLiteral("packagekit-backend"));
}

QUrl ResourcesModel::distroBugReportUrl()
{
    return QUrl(KOSRelease().bugReportUrl());
}
