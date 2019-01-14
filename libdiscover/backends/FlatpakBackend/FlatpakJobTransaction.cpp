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

#include "FlatpakJobTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionThread.h"

#include <QDebug>
#include <QTimer>

extern "C" {
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

FlatpakJobTransaction::FlatpakJobTransaction(FlatpakResource *app, Role role, bool delayStart)
    : Transaction(app->backend(), app, role, {})
    , m_app(app)
{
    setCancellable(true);
    setStatus(QueuedStatus);

    if (!delayStart) {
        QTimer::singleShot(0, this, &FlatpakJobTransaction::start);
    }
}

FlatpakJobTransaction::~FlatpakJobTransaction() = default;

void FlatpakJobTransaction::cancel()
{
    m_appJob->cancel();
    setStatus(CancelledStatus);
}

void FlatpakJobTransaction::start()
{
    setStatus(CommittingStatus);

    // App job will be added every time
    m_appJob = new FlatpakTransactionThread(m_app, role());
    connect(m_appJob, &FlatpakTransactionThread::finished, this, &FlatpakJobTransaction::finishTransaction);
    connect(m_appJob, &FlatpakTransactionThread::progressChanged, this, &FlatpakJobTransaction::setProgress);
    connect(m_appJob, &FlatpakTransactionThread::speedChanged, this, &FlatpakJobTransaction::setDownloadSpeed);
    connect(m_appJob, &FlatpakTransactionThread::passiveMessage, this, &FlatpakJobTransaction::passiveMessage);

    m_appJob->start();
}

void FlatpakJobTransaction::finishTransaction()
{
    if (m_appJob->result()) {
        AbstractResource::State newState = AbstractResource::None;
        switch(role()) {
        case InstallRole:
        case ChangeAddonsRole:
            newState = AbstractResource::Installed;
            break;
        case RemoveRole:
            newState = AbstractResource::None;
            break;
        }

        m_app->setState(newState);

        setStatus(DoneStatus);
    } else {
        if (!m_appJob->errorMessage().isEmpty()) {
            Q_EMIT passiveMessage(m_appJob->errorMessage());
        }
        setStatus(DoneWithErrorStatus);
    }

}
