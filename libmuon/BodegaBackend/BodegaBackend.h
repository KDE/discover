/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef BODEGABACKEND_H
#define BODEGABACKEND_H

#include <resources/AbstractResourcesBackend.h>
#include "libmuonprivate_export.h"
#include <Transaction/Transaction.h>
#include <QHash>

class BodegaResource;
namespace Bodega {
    class Session;
    class NetworkJob;
}

class MUONPRIVATE_EXPORT BodegaBackend : public AbstractResourcesBackend
{
Q_OBJECT
public:
    explicit BodegaBackend(const QString& catalog, const QString& iconName, QObject* parent = 0);
    virtual ~BodegaBackend();
    
    virtual void cancelTransaction(AbstractResource* app);
    virtual void removeApplication(AbstractResource* app);
    virtual void installApplication(AbstractResource* app, const QHash< QString, bool >& addons);
    virtual AbstractResource* resourceByPackageName(const QString& name) const;
    virtual QList< Transaction* > transactions() const;
    virtual QPair< TransactionStateTransition, Transaction* > currentTransactionState() const;
    virtual int updatesCount() const;
    virtual AbstractReviewsBackend* reviewsBackend() const;
    virtual QStringList searchPackageName(const QString& searchText);
    virtual QVector< AbstractResource* > allResources() const;
    virtual AbstractBackendUpdater* backendUpdater() const;

    QList<AbstractResource*> upgradeablePackages();

    Bodega::Session* session() const { return m_session; }
public slots:
    void channelsRetrieved(Bodega::NetworkJob*);
    void resetResources();
    void dataReceived(Bodega::NetworkJob*);

private:
    void createTransaction(Bodega::NetworkJob* install, BodegaResource* res, TransactionAction InstallApp);

    Bodega::Session* m_session;
    QHash<QString, AbstractResource*> m_resourcesByName;
    QList<Transaction*> m_transactions;
};

#endif // BODEGABACKEND_H
