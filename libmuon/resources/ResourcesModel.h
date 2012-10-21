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

#ifndef RESOURCESMODEL_H
#define RESOURCESMODEL_H

#include <QtCore/QModelIndex>
#include <QVector>

#include "libmuonprivate_export.h"
#include "AbstractResourcesBackend.h"

class AbstractResource;
class AbstractResourcesBackend;

class MUONPRIVATE_EXPORT ResourcesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    public:
        enum Roles {
            NameRole = Qt::UserRole,
            IconRole,
            CommentRole,
            ActionRole,
            StateRole,
            RatingRole,
            RatingPointsRole,
            SortableRatingRole,
            ActiveRole,
            ProgressRole,
            ProgressTextRole,
            InstalledRole,
            ApplicationRole,
            UntranslatedNameRole,
            OriginRole,
            CanUpgrade,
            PackageNameRole,
            IsTechnicalRole,
            CategoryRole,
            SectionRole,
            MimeTypes
        };
        explicit ResourcesModel(QObject* parent=0);
        static ResourcesModel* global();
        
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        
        void addResourcesBackend(AbstractResourcesBackend* resources);
        
        AbstractResource* resourceAt(int row) const;
        QModelIndex resourceIndex(AbstractResource* res) const;
        QVector< AbstractResourcesBackend* > backends() const;
        int updatesCount() const;
        
        Q_SCRIPTABLE AbstractResource* resourceByPackageName(const QString& name);
        
    public slots:
        void installApplication(AbstractResource* app, const QHash<QString, bool>& state);
        void installApplication(AbstractResource* app);
        void removeApplication(AbstractResource* app);
        void cancelTransaction(AbstractResource* app);
        void transactionChanged(Transaction* t);

    signals:
        void backendsChanged();
        void updatesCountChanged();
        void searchInvalidated();

        //Transactions forwarding
        void transactionProgressed(Transaction *transaction, int progress);
        void transactionAdded(Transaction *transaction);
        void transactionCancelled(Transaction *transaction);
        void transactionRemoved(Transaction* transaction);
        void transactionsEvent(TransactionStateTransition transition, Transaction* transaction);

    private slots:
        void cleanCaller();
        void resetCaller();
        void updateCaller();
        
    private:
        QVector< AbstractResourcesBackend* > m_backends;
        QVector< QVector<AbstractResource*> > m_resources;
};

#endif // RESOURCESMODEL_H
