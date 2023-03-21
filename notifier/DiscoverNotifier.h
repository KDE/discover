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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class QNetworkConfigurationManager;
#endif
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
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
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

    explicit DiscoverNotifier(QObject *parent = nullptr);
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
        return m_settings;
    }

public Q_SLOTS:
    void recheckSystemUpdateNeeded();
    void showDiscover(const QString &xdgActivationToken);
    void showDiscoverUpdates(const QString &xdgActivationToken);
    void showUpdatesNotification();
    void reboot();
    void foundUpgradeAction(UpgradeAction *action);

Q_SIGNALS:
    void stateChanged();
    bool needsRebootChanged(bool needsReboot);
    void newUpgradeAction(UpgradeAction *action);
    bool busyChanged(bool isBusy);

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QNetworkConfigurationManager *m_manager = nullptr;
#endif
    QPointer<KNotification> m_updatesAvailableNotification;
    UnattendedUpdates *m_unattended = nullptr;
    KConfigWatcher::Ptr m_settingsWatcher;
    QDateTime m_lastUpdate;
    UpdatesSettings *m_settings;
};
