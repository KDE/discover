/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "SnapTransaction.h"
#include "SnapBackend.h"
#include "SnapResource.h"
#include "SnapSocket.h"
#include <QTimer>

SnapTransaction::SnapTransaction(SnapResource* app, SnapJob* job, SnapSocket* socket, Role role)
    : Transaction(app, app, role)
    , m_app(app)
    , m_socket(socket)
{
    setStatus(DownloadingStatus);
    setCancellable(false);
    connect(job, &SnapJob::finished, this, &SnapTransaction::transactionStarted);
    setStatus(SetupStatus);

    // Only needed until /v2/events is fixed upstream
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer, &QTimer::timeout, this, [this](){
        auto job = m_socket->changes(m_changeId);
        connect(job, &SnapJob::finished, this, &SnapTransaction::iterateTransaction);
    });
    job->start();
}

void SnapTransaction::transactionStarted(SnapJob* job)
{
    Q_ASSERT(m_changeId.isEmpty());

    if (!job->isSuccessful()) {
        qWarning() << "non-successful transaction" << job->statusCode();
        setStatus(DoneStatus);
        return;
    }

    auto data = job->data();

    m_changeId = data.value(QLatin1String("change")).toString();

    setStatus(DownloadingStatus);
    m_timer->start();
}

void SnapTransaction::iterateTransaction(SnapJob* job)
{
    auto res = job->result().toObject();

    if (res.value(QLatin1String("ready")).toBool()) {
        finishTransaction();
    }
}

void SnapTransaction::cancel()
{
    Q_UNREACHABLE();
}

void SnapTransaction::finishTransaction()
{
    delete m_timer;
    m_app->refreshState();
    setStatus(DoneStatus);
}
