// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "KDELinuxBackend.h"

#include <chrono>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QJsonDocument>
#include <QProcess>

#include <resources/StandardBackendUpdater.h>

#include "KDELinuxResource.h"
#include "KDELinuxTransaction.h"
#include "debug.h"
#include "polkit/deletelater.h"

DISCOVER_BACKEND_PLUGIN(KDELinuxBackend)

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

KDELinuxBackend::KDELinuxBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
{
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &KDELinuxBackend::updatesCountChanged);

    QMetaObject::invokeMethod(this, &KDELinuxBackend::checkForUpdates, Qt::QueuedConnection);
}

void KDELinuxBackend::checkForUpdates()
{
    if (m_fetching) {
        return;
    }

    m_fetching = true;
    Q_EMIT fetchingChanged();

    auto msg = QDBusMessage::createMethodCall(u"org.kde.discover.kde.linux"_s, u"/"_s, u"org.kde.discover.kde.linux"_s, u"check_new"_s);
    constexpr std::chrono::milliseconds timeout = 8s;
    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg, timeout.count()));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const auto deferFetching = qScopeGuard([this] {
            m_fetching = false;
            Q_EMIT updatesCountChanged();
            Q_EMIT fetchingChanged();
        });
        watcher->deleteLater();
        QDBusReply<QString> reply = *watcher;
        if (!reply.isValid() || reply.error().isValid()) {
            qCWarning(KDELINUX) << "Failed to check_new" << reply.error().message();
            Q_EMIT passiveMessage(reply.error().message());
            m_resource = nullptr;
            return;
        }
        const auto updateVersion = reply.value();
        if (updateVersion.isEmpty()) {
            m_resource = nullptr;
            return;
        }
        m_resource = makeDeleteLaterShared<KDELinuxResource>(updateVersion, this);
    });
}

int KDELinuxBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool KDELinuxBackend::isValid() const
{
    return true;
}

AbstractBackendUpdater *KDELinuxBackend::backendUpdater() const
{
    return m_updater;
}

QString KDELinuxBackend::displayName() const
{
    return u"KDE Linux"_s;
}

AbstractReviewsBackend *KDELinuxBackend::reviewsBackend() const
{
    qCWarning(KDELINUX) << "Reviews not supported on kde-linux backend";
    return nullptr;
}

ResultsStream *KDELinuxBackend::search(const Filters &search)
{
    if (!m_resource) {
        return new ResultsStream(QLatin1String("kde-linux-search-stream"), {});
    }
    return new ResultsStream(QLatin1String("kde-linux-search-stream"), {StreamResult(m_resource.get(), 0)});
}

bool KDELinuxBackend::isFetching() const
{
    return m_fetching;
}

Transaction *KDELinuxBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    auto deferReset = qScopeGuard([this] {
        m_resource = nullptr;
    });
    return new KDELinuxTransaction(this, m_resource);
}

Transaction *KDELinuxBackend::removeApplication(AbstractResource *app)
{
    qCWarning(KDELINUX) << "Removing applications not supported on kde-linux backend";
    return nullptr;
}

#include "KDELinuxBackend.moc"
