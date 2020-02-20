/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "FlatpakNotifier.h"

#include <glib.h>

#include <QDebug>
#include <QTimer>
#include <QtConcurrentRun>
#include <QFutureWatcher>

static void installationChanged(GFileMonitor *monitor, GFile *child, GFile *other_file, GFileMonitorEvent event_type, gpointer self)
{
    Q_UNUSED(monitor);
    Q_UNUSED(child);
    Q_UNUSED(other_file);
    Q_UNUSED(event_type);

    FlatpakNotifier *notifier = (FlatpakNotifier*)self;
    if (!notifier) {
        return;
    }

    notifier->recheckSystemUpdateNeeded();
}

FlatpakNotifier::FlatpakNotifier(QObject* parent)
    : BackendNotifierModule(parent)
    , m_cancellable(g_cancellable_new())
{
    QTimer *dailyCheck = new QTimer(this);
    dailyCheck->setInterval(24 * 60 * 60 * 1000); //refresh at least once every day
    connect(dailyCheck, &QTimer::timeout, this, &FlatpakNotifier::recheckSystemUpdateNeeded);
}

FlatpakNotifier::Installation::~Installation()
{
    if (m_monitor)
        g_object_unref(m_monitor);
    if (m_installation)
        g_object_unref(m_installation);
}

FlatpakNotifier::~FlatpakNotifier()
{
    g_object_unref(m_cancellable);
}

void FlatpakNotifier::recheckSystemUpdateNeeded()
{
    g_autoptr(GError) error = nullptr;

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qWarning() << "Failed to setup flatpak installations: " << error->message;
    } else {
        // Load updates from remote repositories
        loadRemoteUpdates(m_system.m_installation);
        loadRemoteUpdates(m_user.m_installation);
    }
}

void FlatpakNotifier::onFetchUpdatesFinished(FlatpakInstallation *flatpakInstallation, GPtrArray *updates)
{
    bool changed = false;
    bool hasUpdates = false;

    g_autoptr(GPtrArray) fetchedUpdates = updates;

    for (uint i = 0; !hasUpdates && i < fetchedUpdates->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(fetchedUpdates, i));
        const QString refName = QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(ref)));
        // FIXME right now I can't think of any other filter than this, in FlatpakBackend updates are matched
        // with apps so .Locale/.Debug subrefs are not shown and updated automatically. Also this will show
        // updates for refs we don't show in Discover if appstream metadata or desktop file for them is not found
        if (refName.endsWith(QLatin1String(".Locale")) || refName.endsWith(QLatin1String(".Debug"))) {
            continue;
        }
        hasUpdates = true;
    }

    if (flatpak_installation_get_is_user(flatpakInstallation)) {
        changed = m_user.m_hasUpdates != hasUpdates;
        m_user.m_hasUpdates = hasUpdates;
    } else {
        changed = m_system.m_hasUpdates != hasUpdates;
        m_system.m_hasUpdates = hasUpdates;
    }

    if (changed) {
        Q_EMIT foundUpdates();
    }
}

void FlatpakNotifier::loadRemoteUpdates(FlatpakInstallation *installation)
{
    auto fw = new QFutureWatcher<GPtrArray *>(this);
    connect(fw, &QFutureWatcher<GPtrArray *>::finished, this, [this, installation, fw](){
        g_autoptr(GPtrArray) refs = fw->result();
        if (refs)
            onFetchUpdatesFinished(installation, refs);
        fw->deleteLater();
    });
    fw->setFuture(QtConcurrent::run( [installation]() -> GPtrArray * {
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) localError = nullptr;
        GPtrArray *refs = flatpak_installation_list_installed_refs_for_update(installation, cancellable, &localError);
        if (!refs) {
            qWarning() << "Failed to get list of installed refs for listing updates: " << localError->message;
        }
        return refs;
    }));
}

bool FlatpakNotifier::hasUpdates()
{
    return m_system.m_hasUpdates || m_user.m_hasUpdates;
}

bool FlatpakNotifier::setupFlatpakInstallations(GError **error)
{
    if (!m_system.m_installation) {
        m_system.m_installation = flatpak_installation_new_system(m_cancellable, error);
        if (!m_system.m_installation) {
            return false;
        }
    }

    if (!m_user.m_installation) {
        m_user.m_installation = flatpak_installation_new_user(m_cancellable, error);
        if (!m_user.m_installation) {
            return false;
        }
    }

    if (!m_system.m_monitor) {
        m_system.m_monitor = flatpak_installation_create_monitor(m_system.m_installation, m_cancellable, error);
        if (!m_system.m_monitor) {
            return false;
        }

        g_signal_connect(m_system.m_monitor, "changed", G_CALLBACK(installationChanged), this);
    }

    if (!m_user.m_monitor) {
        m_user.m_monitor = flatpak_installation_create_monitor(m_user.m_installation, m_cancellable, error);
        if (!m_user.m_monitor) {
            return false;
        }

        g_signal_connect(m_user.m_monitor, "changed", G_CALLBACK(installationChanged), this);
    }

    return true;
}
