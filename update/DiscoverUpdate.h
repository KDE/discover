/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QSet>
#include <QUrl>

#include <Transaction/Transaction.h>

class AbstractResource;
class ResourcesUpdatesModel;

class DiscoverUpdate : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverUpdate();
    ~DiscoverUpdate() override;

    void setOfflineUpdates(bool offline)
    {
        m_offlineUpdates = offline;
    }

private:
    void start();
    void transactionStatusChanged(Transaction::Status status);

    ResourcesUpdatesModel *const m_resourcesUpdatesModel;
    bool m_done = false;
    bool m_offlineUpdates = false;
};
