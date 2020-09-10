/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DISCOVERNOTIFIERMODULE_H
#define DISCOVERNOTIFIERMODULE_H

#include <BackendNotifierModule.h>
#include <QStringList>
#include <QTimer>
#include <QPointer>

#include <KNotification>

class QNetworkConfigurationManager;
class KNotification;

class DiscoverNotifier : public QObject
{
Q_OBJECT
Q_PROPERTY(QStringList modules READ loadedModules CONSTANT)
Q_PROPERTY(QString iconName READ iconName NOTIFY stateChanged)
Q_PROPERTY(QString message READ message NOTIFY stateChanged)
Q_PROPERTY(State state READ state NOTIFY stateChanged)
Q_PROPERTY(bool needsReboot READ needsReboot NOTIFY needsRebootChanged)
public:
    enum State {
        NoUpdates,
        NormalUpdates,
        SecurityUpdates,
        RebootRequired,
        Offline,
    };
    Q_ENUM(State)

    explicit DiscoverNotifier(QObject* parent = nullptr);
    ~DiscoverNotifier() override;

    State state() const;
    QString iconName() const;
    QString message() const;
    bool hasUpdates() const { return m_hasUpdates; }
    bool hasSecurityUpdates() const { return m_hasSecurityUpdates; }

    QStringList loadedModules() const;
    bool needsReboot() const { return m_needsReboot; }

public Q_SLOTS:
    void recheckSystemUpdateNeeded();
    void showDiscover();
    void showDiscoverUpdates();
    void showUpdatesNotification();
    void reboot();
    void foundUpgradeAction(UpgradeAction* action);

Q_SIGNALS:
    void stateChanged();
    bool needsRebootChanged(bool needsReboot);
    void newUpgradeAction(UpgradeAction* action);

private:
    void showRebootNotification();
    void updateStatusNotifier();

    QList<BackendNotifierModule*> m_backends;
    QTimer m_timer;
    bool m_hasSecurityUpdates = false;
    bool m_hasUpdates = false;
    bool m_needsReboot = false;
    QNetworkConfigurationManager* m_manager = nullptr;
    QPointer<KNotification> m_updatesAvailableNotification;
};

#endif //ABSTRACTKDEDMODULE_H
