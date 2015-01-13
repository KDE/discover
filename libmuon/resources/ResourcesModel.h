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

#include "libMuonCommon_export.h"
#include "AbstractResourcesBackend.h"

class AbstractResource;
class AbstractResourcesBackend;

class MUONCOMMON_EXPORT ResourcesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool fetching READ isFetching NOTIFY fetchingChanged)
    public:
        enum Roles {
            NameRole = Qt::UserRole,
            IconRole,
            CommentRole,
            StateRole,
            RatingRole,
            RatingPointsRole,
            SortableRatingRole,
            ActiveRole,
            InstalledRole,
            ApplicationRole,
            OriginRole,
            CanUpgrade,
            PackageNameRole,
            IsTechnicalRole,
            CategoryRole,
            SectionRole,
            MimeTypes
        };
        /** This constructor should be only used by unit tests.
         *  @p backendName defines what backend will be loaded when the backend is constructed.
         */
        ResourcesModel(const QString& backendName, QObject* parent = 0);
        static ResourcesModel* global();
        virtual ~ResourcesModel();
        
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        
        AbstractResource* resourceAt(int row) const;
        QModelIndex resourceIndex(AbstractResource* res) const;
        QVector< AbstractResourcesBackend* > backends() const;
        int updatesCount() const;
        virtual QMap< int, QVariant > itemData(const QModelIndex& index) const;
        
        Q_SCRIPTABLE AbstractResource* resourceByPackageName(const QString& name);

        void integrateMainWindow(MuonMainWindow* w);
        
        bool isFetching() const;
        QList<QAction*> messageActions() const;
        
        virtual QHash<int, QByteArray> roleNames() const;

    public slots:
        void installApplication(AbstractResource* app, AddonList addons);
        void installApplication(AbstractResource* app);
        void removeApplication(AbstractResource* app);
        void cancelTransaction(AbstractResource* app);

    signals:
        void fetchingChanged();
        void allInitialized();
        void backendsChanged();
        void updatesCountChanged();
        void searchInvalidated();

    private slots:
        void resetBackend(AbstractResourcesBackend* backend);
        void cleanBackend(AbstractResourcesBackend* backend);
        void callerFetchingChanged();
        void updateCaller();
        void registerAllBackends();
        void resourceChangedByTransaction(Transaction* t);

    private:
        int rowsBeforeBackend(AbstractResourcesBackend* backend, QVector<QVector<AbstractResource*>>::iterator& backendsResources);

        ///@p initialize tells if all backends load will be triggered on construction
        explicit ResourcesModel(QObject* parent=0, bool initialize = true);
        void init(bool initialize);
        void addResourcesBackend(AbstractResourcesBackend* resources);
        void registerBackendByName(const QString& name);

        QVector< AbstractResourcesBackend* > m_backends;
        QVector< QVector<AbstractResource*> > m_resources;
        int m_initializingBackends;
        MuonMainWindow* m_mainwindow;

        static ResourcesModel* s_self;
};

#endif // RESOURCESMODEL_H
