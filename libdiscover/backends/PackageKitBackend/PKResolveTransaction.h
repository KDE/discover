/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef PKRESOLVETRANSACTION_H
#define PKRESOLVETRANSACTION_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <PackageKit/Transaction>

class PackageKitBackend;

class PKResolveTransaction : public QObject
{
    Q_OBJECT
    public:
        PKResolveTransaction(PackageKitBackend* backend);

        void start();
        void addPackageNames(const QStringList &packageNames);

    Q_SIGNALS:
        void allFinished();
        void started();

    private:
        void transactionFinished(PackageKit::Transaction::Exit exit);

        QTimer m_floodTimer;
        QStringList m_packageNames;
        QVector<PackageKit::Transaction*> m_transactions;
        PackageKitBackend* const m_backend;
};

#endif
