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

#ifndef OCSBACKEND_H
#define OCSBACKEND_H

#include <resources/AbstractResourcesBackend.h>
#include <attica/provider.h>

class OCSResource;
namespace Attica { class ProviderManager; }

class OCSBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    public:
        explicit OCSBackend(QObject* parent = 0);
        
        virtual QVector< AbstractResource* > allResources() const;
        virtual void installApplication(AbstractResource* app) {}
        virtual void installApplication(AbstractResource* app, const QHash< QString, bool >& addons) {}
        virtual void cancelTransaction(AbstractResource* app) {}
        virtual QPair< TransactionStateTransition, Transaction* > currentTransactionState() const { return QPair<TransactionStateTransition, Transaction*>(); }
        virtual bool providesResouce(AbstractResource* resource) const;
        virtual void removeApplication(AbstractResource* app) {}
        virtual AbstractResource* resourceByPackageName(const QString& name) const { return 0; }
        virtual AbstractReviewsBackend* reviewsBackend() const { return 0; }
        virtual QStringList searchPackageName(const QString& searchText);
        virtual QList< Transaction* > transactions() const { return QList<Transaction*>(); }
        virtual int updatesCount() const { return 0; }

    private slots:
        void providersLoaded();
        void categoriesLoaded(Attica::BaseJob*);
        void loadContents(Attica::BaseJob*);


    private:
        Attica::ProviderManager* m_manager;
        Attica::Provider m_provider;
        Attica::Category::List m_categories;
        QVector<OCSResource*> m_resources;
        int m_currentPage;
};

#endif // OCSBACKEND_H
