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
#include <Transaction/AddonList.h>
#include <Transaction/Transaction.h>
#include <QHash>
#include <QVariantList>

class BodegaResource;
namespace Bodega {
    class Session;
    class NetworkJob;
    class UninstallJob;
}

class MUONPRIVATE_EXPORT BodegaBackend : public AbstractResourcesBackend
{
Q_OBJECT
Q_INTERFACES(AbstractResourcesBackend)
public:
    explicit BodegaBackend(QObject* parent, const QVariantList& args);
    virtual ~BodegaBackend();
    
    virtual void cancelTransaction(AbstractResource* app);
    virtual void removeApplication(AbstractResource* app);
    virtual void installApplication(AbstractResource* app, AddonList addons);
    virtual void installApplication(AbstractResource* app) { installApplication(app, AddonList()); }
    virtual AbstractResource* resourceByPackageName(const QString& name) const;
    virtual int updatesCount() const;
    virtual AbstractReviewsBackend* reviewsBackend() const;
    virtual QList<AbstractResource*> searchPackageName(const QString& searchText);
    virtual QVector< AbstractResource* > allResources() const;
    virtual AbstractBackendUpdater* backendUpdater() const;
    virtual bool isValid() const { return true; } // No external file dependencies that could cause runtime errors

    QList<AbstractResource*> upgradeablePackages() const;

    Bodega::Session* session() const { return m_session; }
    QString icon() const { return m_icon; }

public slots:
    void channelsRetrieved(Bodega::NetworkJob*);
    void resetResources();
    void dataReceived(Bodega::NetworkJob*);
    void removeTransaction(Bodega::NetworkJob* job);
    void removeTransaction(Bodega::UninstallJob* job);
    void removeTransactionGeneric(QObject* job);

private:
    Bodega::Session* m_session;
    QHash<QString, AbstractResource*> m_resourcesByName;
    QList<Transaction*> m_transactions;
    QString m_channel;
    QString m_icon;
};

#endif // BODEGABACKEND_H
