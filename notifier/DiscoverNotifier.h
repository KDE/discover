/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef DISCOVERNOTIFIERMODULE_H
#define DISCOVERNOTIFIERMODULE_H

#include <BackendNotifierModule.h>
#include <QStringList>
#include <QTimer>

class KStatusNotifierItem;

class DiscoverNotifier : public QObject
{
Q_OBJECT
Q_PROPERTY(QStringList modules READ loadedModules CONSTANT)
Q_PROPERTY(QString iconName READ iconName NOTIFY updatesChanged)
Q_PROPERTY(QString message READ message NOTIFY updatesChanged)
Q_PROPERTY(State state READ state NOTIFY updatesChanged)
Q_PROPERTY(bool needsReboot READ needsReboot NOTIFY needsRebootChanged)
public:
    enum State {
        NoUpdates,
        NormalUpdates,
        SecurityUpdates,
        RebootRequired
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
    void configurationChanged();
    void recheckSystemUpdateNeeded();
    void showDiscover();
    void showDiscoverUpdates();
    void showUpdatesNotification();
    void reboot();
    void foundUpgradeAction(UpgradeAction* action);

Q_SIGNALS:
    void updatesChanged();
    bool needsRebootChanged(bool needsReboot);
    void newUpgradeAction(UpgradeAction* action);

private:
    void showRebootNotification();
    void updateStatusNotifier();

    QList<BackendNotifierModule*> m_backends;
    bool m_verbose = false;
    QTimer m_timer;
    bool m_hasSecurityUpdates = false;
    bool m_hasUpdates = false;
    bool m_needsReboot = false;
};

#endif //ABSTRACTKDEDMODULE_H
