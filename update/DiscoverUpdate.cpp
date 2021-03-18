/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverUpdate.h"
#include <QCoreApplication>
#include <QDebug>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>

DiscoverUpdate::DiscoverUpdate()
    : QObject(nullptr)
    , m_resourcesUpdatesModel(new ResourcesUpdatesModel)
{
    connect(m_resourcesUpdatesModel, &ResourcesUpdatesModel::passiveMessage, this, [](const QString &message) {
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

    m_resourcesUpdatesModel->setOfflineUpdates(m_offlineUpdates);
    qDebug() << "ready" << ResourcesModel::global()->updatesCount();
    m_resourcesUpdatesModel->prepare();
    qDebug() << "steady" << m_resourcesUpdatesModel->rowCount({});
    m_resourcesUpdatesModel->updateAll();

    auto transaction = m_resourcesUpdatesModel->transaction();
    connect(transaction, &Transaction::statusChanged, this, &DiscoverUpdate::transactionStatusChanged);

    qDebug() << "go!" << transaction;
}

void DiscoverUpdate::transactionStatusChanged(Transaction::Status status)
{
    m_done = true;
    qDebug() << "status!" << status << ResourcesModel::global()->updatesCount();
    if (status == Transaction::DoneStatus || status == Transaction::DoneWithErrorStatus) {
        const bool withError = status == Transaction::DoneWithErrorStatus;
        QCoreApplication::instance()->exit(withError);
    }
}
