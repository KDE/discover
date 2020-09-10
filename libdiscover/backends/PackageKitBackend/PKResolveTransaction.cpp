/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PKResolveTransaction.h"
#include <PackageKit/Daemon>
#include "PackageKitBackend.h"

#include <QDebug>

PKResolveTransaction::PKResolveTransaction(PackageKitBackend* backend)
    : m_backend(backend)
{
    m_floodTimer.setInterval(1000);
    m_floodTimer.setSingleShot(true);
    connect(&m_floodTimer, &QTimer::timeout, this, &PKResolveTransaction::start);
}

void PKResolveTransaction::start()
{
    Q_EMIT started();

    PackageKit::Transaction * tArch = PackageKit::Daemon::resolve(m_packageNames, PackageKit::Transaction::FilterArch);
    connect(tArch, &PackageKit::Transaction::package, m_backend, &PackageKitBackend::addPackageArch);
    connect(tArch, &PackageKit::Transaction::errorCode, m_backend, &PackageKitBackend::transactionError);

    PackageKit::Transaction * tNotArch = PackageKit::Daemon::resolve(m_packageNames, PackageKit::Transaction::FilterNotArch);
    connect(tNotArch, &PackageKit::Transaction::package, m_backend, &PackageKitBackend::addPackageNotArch);
    connect(tNotArch, &PackageKit::Transaction::errorCode, m_backend, &PackageKitBackend::transactionError);

    m_transactions = {tArch, tNotArch};

    foreach(PackageKit::Transaction* t, m_transactions) {
        connect(t, &PackageKit::Transaction::finished, this, &PKResolveTransaction::transactionFinished);
    }
}

void PKResolveTransaction::transactionFinished(PackageKit::Transaction::Exit exit)
{
    PackageKit::Transaction* t = qobject_cast<PackageKit::Transaction*>(sender());
    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "failed" << exit << t;
    }

    m_transactions.removeAll(t);
    if (m_transactions.isEmpty()) {
        Q_EMIT allFinished();
        deleteLater();
    }
}

void PKResolveTransaction::addPackageNames(const QStringList& packageNames)
{
    m_packageNames += packageNames;
    m_packageNames.removeDuplicates();
    m_floodTimer.start();
}
