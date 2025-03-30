/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <BackendNotifierModule.h>
#include <QPointer>
#include <QStringList>
#include <QTimer>

#include <KConfigWatcher>
#include <KNotification>

class KNotification;
class UnattendedUpdates;
class UpdatesSettings;

class DiscoverNotifier : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList modules READ loadedModules CONSTANT)
    Q_PROPERTY(QString iconName READ iconName NOTIFY stateChanged)
    Q_PROPERTY(QString message READ message NOTIFY stateChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool needsReboot READ needsReboot NOTIFY needsRebootChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool isSystemUpdateable READ isSystemUpdateable NOTIFY stateChanged)
public:
    enum State {
        NoUpdates,
        NormalUpdates,
        SecurityUpdates,
        Busy,
        RebootRequired,
        Offline,
    };
    Q_ENUM(State)

    explicit DiscoverNotifier(const std::chrono::seconds &checkDelay, QObject *parent = nullptr);
    ~DiscoverNotifier() override;

    State state() const;
    QString iconName() const;
    QString message() const;
    bool hasUpdates() const
    {
        return m_hasUpdates;
    }
    bool hasSecurityUpdates() const
    {
        return m_hasSecurityUpdates;
    }
    bool isSystemUpdateable() const;

    QStringList loadedModules() const;
    bool needsReboot() const
    {
        return m_needsReboot;
    }

    void setBusy(bool isBusy);
    bool isBusy() const
    {
        return m_isBusy;
    }
    UpdatesSettings *settings() const
    {
        return m_settings.get();
    }
    void startUnattendedUpdates();

public Q_SLOTS:
    void recheckSystemUpdateNeededAndNotifyApp();
    void recheckSystemUpdateNeeded();
    void showDiscover(const QString &xdgActivationToken);
    void showDiscoverUpdates(const QString &xdgActivationToken);
    void showUpdatesNotification();
    void rebootPrompt();
    void shutdownPrompt();
    void promptAll();
    void foundUpgradeAction(UpgradeAction *action);

Q_SIGNALS:
    void stateChanged();
    bool needsRebootChanged(bool needsReboot);
    void newUpgradeAction(UpgradeAction *action);
    bool busyChanged();

private:
    void showRebootNotification();
    void updateStatusNotifier();
    void refreshUnattended();

    bool notifyAboutUpdates() const;

    QList<BackendNotifierModule *> m_backends;
    QTimer m_timer;
    bool m_hasSecurityUpdates = false;
    bool m_hasUpdates = false;
    bool m_needsReboot = false;
    bool m_isBusy = false;
    QPointer<KNotification> m_updatesAvailableNotification;
    std::unique_ptr<UnattendedUpdates> m_unattended;
    KConfigWatcher::Ptr m_settingsWatcher;
    QDateTime m_lastUpdate;
    std::unique_ptr<UpdatesSettings> m_settings;
};
