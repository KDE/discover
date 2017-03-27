/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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
#ifndef FLATPAKNOTIFIER_H
#define FLATPAKNOTIFIER_H

#include <BackendNotifierModule.h>

extern "C" {
#include <flatpak.h>
}

class FlatpakNotifier : public BackendNotifierModule
{
Q_OBJECT
Q_PLUGIN_METADATA(IID "org.kde.discover.BackendNotifierModule")
Q_INTERFACES(BackendNotifierModule)
public:
    explicit FlatpakNotifier(QObject* parent = nullptr);
    ~FlatpakNotifier() override;

    bool isSystemUpToDate() const override;
    void recheckSystemUpdateNeeded() override;
    uint securityUpdatesCount() override;
    uint updatesCount() override;

public Q_SLOTS:
    void checkUpdates();
    void onFetchUpdatesFinished(FlatpakInstallation *flatpakInstallation, GPtrArray *updates);

private:
    void loadRemoteUpdates(FlatpakInstallation *flatpakInstallation);
    bool setupFlatpakInstallations(GError **error);

    uint m_userInstallationUpdates;
    uint m_systemInstallationUpdates;
    GCancellable *m_cancellable;
    GFileMonitor *m_userInstallationMonitor = nullptr;
    GFileMonitor *m_systemInstallationMonitor = nullptr;
    FlatpakInstallation *m_flatpakInstallationUser = nullptr;
    FlatpakInstallation *m_flatpakInstallationSystem = nullptr;
};

#endif
