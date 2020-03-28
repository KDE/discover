/*
 *   Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "DiscoverUpdate.h"
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>
#include <QCoreApplication>

DiscoverUpdate::DiscoverUpdate()
    : QObject(nullptr)
    , m_resourcesUpdatesModel(new ResourcesUpdatesModel)
{
    connect(m_resourcesUpdatesModel, &ResourcesUpdatesModel::passiveMessage, this, [] (const QString &message) {
        qWarning() << "message" << message;
    });
    connect(ResourcesModel::global(), &ResourcesModel::fetchingChanged, this, &DiscoverUpdate::start);
    connect(m_resourcesUpdatesModel, &ResourcesUpdatesModel::progressingChanged, this, &DiscoverUpdate::start);
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &DiscoverUpdate::start);
}

DiscoverUpdate::~DiscoverUpdate() = default;

void DiscoverUpdate::start()
{
    if (m_resourcesUpdatesModel->isProgressing() || ResourcesModel::global()->isFetching() || m_done)
        return;

    qDebug() << "ready" << ResourcesModel::global()->updatesCount();
    m_resourcesUpdatesModel->prepare();
    qDebug() << "steady" << m_resourcesUpdatesModel->rowCount({});
    m_resourcesUpdatesModel->updateAll();

    auto transaction = m_resourcesUpdatesModel->transaction();
    connect(transaction, &Transaction::statusChanged, this, &DiscoverUpdate::transactionStatusChanged);

    qDebug() <<"go!" << transaction;
}

void DiscoverUpdate::transactionStatusChanged(Transaction::Status status)
{
    m_done = true;
    qDebug() <<"status!" << status << ResourcesModel::global()->updatesCount();
    if (status == Transaction::DoneStatus || status == Transaction::DoneWithErrorStatus) {
        const bool withError = status == Transaction::DoneWithErrorStatus;
        QCoreApplication::instance()->exit(withError);
    }
}
