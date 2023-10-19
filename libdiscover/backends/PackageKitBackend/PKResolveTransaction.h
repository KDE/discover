/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <PackageKit/Transaction>
#include <QList>
#include <QObject>
#include <QTimer>

class PackageKitBackend;

class PKResolveTransaction : public QObject
{
    Q_OBJECT
public:
    PKResolveTransaction(PackageKitBackend *backend);

    void start();
    void addPackageNames(const QStringList &packageNames);

Q_SIGNALS:
    void allFinished();
    void started();

private:
    void transactionFinished(PackageKit::Transaction::Exit exit);

    QTimer m_floodTimer;
    QStringList m_packageNames;
    QList<PackageKit::Transaction *> m_transactions;
    PackageKitBackend *const m_backend;
};
