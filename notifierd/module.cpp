// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <QNetworkInformation>
#include <QTimer>

#include <KConfigWatcher>
#include <KDEDModule>
#include <KIdleTime>
#include <KLibexec>
#include <KPluginFactory>

#include "Login1ManagerInterface.h"
#include "updatessettings.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace
{
constexpr auto SETTLE_TIME = 1s;

// Wraps a KIdleTime id to ensure it is released whenever necessary.
class IdleHandle
{
public:
    IdleHandle(const std::chrono::milliseconds &idleTimeout)
        : m_id(KIdleTime::instance()->addIdleTimeout(int(idleTimeout.count())))
    {
    }
    ~IdleHandle()
    {
        KIdleTime::instance()->removeIdleTimeout(m_id);
    }
    Q_DISABLE_COPY_MOVE(IdleHandle)
    int m_id;
};
} // namespace

class DiscoverUnattendedUpdatesWatcher : public QObject
{
    Q_OBJECT
public:
    DiscoverUnattendedUpdatesWatcher(UpdatesSettings *settings, const std::chrono::milliseconds &idleTimeout)
        : m_idle(idleTimeout)
    {
        connect(KIdleTime::instance(), &KIdleTime::timeoutReached, this, [this, settings](int identifier) {
            if (!m_idle || identifier != m_idle->m_id) {
                return;
            }

            settings->read();
            if (!settings->useUnattendedUpdates()) {
                return;
            }

            Q_EMIT startNotifier();
        });
    }

Q_SIGNALS:
    void startNotifier();

private:
    std::optional<IdleHandle> m_idle;
};

class DiscoverNotifierWatcher : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverNotifierWatcher(UpdatesSettings *settings, QObject *parent = nullptr)
        : QObject(parent)
        , m_settings(settings)
    {
        connect(&m_compressionTimer, &QTimer::timeout, this, &DiscoverNotifierWatcher::recheckInternal);

        QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability | QNetworkInformation::Feature::TransportMedium);
        if (auto info = QNetworkInformation::instance()) {
            connect(info, &QNetworkInformation::reachabilityChanged, this, &DiscoverNotifierWatcher::recheck);
            connect(info, &QNetworkInformation::transportMediumChanged, this, &DiscoverNotifierWatcher::recheck);
            connect(info, &QNetworkInformation::isBehindCaptivePortalChanged, this, &DiscoverNotifierWatcher::recheck);
        } else {
            qWarning() << "QNetworkInformation has no backend. Is NetworkManager.service running?";
        }

        auto login1 = new OrgFreedesktopLogin1ManagerInterface(u"org.freedesktop.login1"_s, u"/org/freedesktop/login1"_s, QDBusConnection::systemBus(), this);
        connect(login1, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, this, &DiscoverNotifierWatcher::recheck);

#warning todo how exactly did the notifier work when the user hadnt logged out in a day... do we need a long running timer maybe
    }

    void recheck()
    {
        if (m_compressionTimer.isActive()) {
            return;
        }
        m_compressionTimer.start(SETTLE_TIME);
    }

Q_SIGNALS:
    void startNotifier();

private:
    void recheckInternal()
    {
        if (!shouldCheckForUpdates()) {
            return;
        }

        Q_EMIT startNotifier();
    }

    [[nodiscard]] bool shouldCheckForUpdates() const
    {
        m_settings->read();
        if (m_settings->requiredNotificationInterval() < 0) {
            return false;
        }

        // To configure to a random value, execute:
        // kwriteconfig5 --file PlasmaDiscoverUpdates --group Global --key RequiredNotificationInterval 3600
        const QDateTime earliestNextNotificationTime = m_settings->lastNotificationTime().addSecs(m_settings->requiredNotificationInterval());
        return !(earliestNextNotificationTime.isValid() && earliestNextNotificationTime > QDateTime::currentDateTimeUtc());
    }

    UpdatesSettings *m_settings;
    QTimer m_compressionTimer;
};

class DiscoverNotifierModule : public KDEDModule
{
    Q_OBJECT
public:
    DiscoverNotifierModule(QObject *parent, const QList<QVariant> & /*args*/)
        : KDEDModule(parent)
    {
        // Let the system settle before doing anything.
        QTimer::singleShot(SETTLE_TIME, &m_notificationWatcher, &DiscoverNotifierWatcher::recheck);

        const auto startNotifier = [] {
#warning completely untested
            QProcess notifier;
            notifier.setProgram(KLibexec::path(u"plasma-discover/DiscoverNotifier"_s));
            notifier.startDetached();
        };
        connect(&m_notificationWatcher, &DiscoverNotifierWatcher::startNotifier, this, startNotifier);
        connect(&m_unattendedWatcher, &DiscoverUnattendedUpdatesWatcher::startNotifier, this, startNotifier);
    }

    UpdatesSettings m_settings;
    DiscoverNotifierWatcher m_notificationWatcher{&m_settings};
    DiscoverUnattendedUpdatesWatcher m_unattendedWatcher{&m_settings, 15min};
};

K_PLUGIN_CLASS_WITH_JSON(DiscoverNotifierModule, "discover-notifier.json")

#include "module.moc"
