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

#include "FlatpakTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionJob.h"

#include <Transaction/TransactionModel.h>

#include <QDebug>
#include <QTimer>

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, Role role)
    : FlatpakTransaction(installation, app, {}, role)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, const AddonList &addons, Transaction::Role role)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
    , m_installation(installation)
{
    setCancellable(true);

    TransactionModel::global()->addTransaction(this);

    QTimer::singleShot(0, this, &FlatpakTransaction::start);
}

FlatpakTransaction::~FlatpakTransaction()
{
    delete m_job;
}

void FlatpakTransaction::cancel()
{
    m_job->cancel();
    TransactionModel::global()->cancelTransaction(this);
}

void FlatpakTransaction::start()
{
    m_job = new FlatpakTransactionJob(m_installation, m_app, role());
    connect(m_job, &FlatpakTransactionJob::jobFinished, this, &FlatpakTransaction::onJobFinished);
    connect(m_job, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onJobProgressChanged);
    m_job->start();
}

void FlatpakTransaction::onJobFinished(bool success)
{
    if (success) {
        finishTransaction();
    }
}

void FlatpakTransaction::onJobProgressChanged(int progress)
{
    setProgress(progress);
}

void FlatpakTransaction::finishTransaction()
{
    setStatus(DoneStatus);
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
//     m_app->setAddons(addons());
    TransactionModel::global()->removeTransaction(this);
    deleteLater();
}
