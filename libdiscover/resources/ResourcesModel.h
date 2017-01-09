/***************************************************************************
 *   Copyright ?? 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include <QSet>
#include <QVector>
#include <QTimer>

#include "discovercommon_export.h"
#include "AbstractResourcesBackend.h"

class DISCOVERCOMMON_EXPORT AggregatedResultsStream : public ResultsStream
{
Q_OBJECT
public:
    AggregatedResultsStream(const QSet<ResultsStream*>& streams);

Q_SIGNALS:
    void finished();

private:
    void addResults(const QVector<AbstractResource*>& res);
    void emitResults();
    void destruction(QObject* obj);

    QSet<QObject*> m_streams;
    QVector<AbstractResource*> m_results;
    QTimer m_delayedEmission;
};

class DISCOVERCOMMON_EXPORT ResourcesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY fetchingChanged)
    public:
        /** This constructor should be only used by unit tests.
         *  @p backendName defines what backend will be loaded when the backend is constructed.
         */
        explicit ResourcesModel(const QString& backendName, QObject* parent = nullptr);
        static ResourcesModel* global();
        ~ResourcesModel() override;
        
        QVector< AbstractResourcesBackend* > backends() const;
        int updatesCount() const;
        
        void integrateActions(KActionCollection* w);
        
        bool isBusy() const;
        bool isFetching() const;
        QList<QAction*> messageActions() const;
        
        Q_SCRIPTABLE bool isExtended(const QString &id);

        AggregatedResultsStream* findResourceByPackageName(const QUrl& search);
        AggregatedResultsStream* search(const AbstractResourcesBackend::Filters &search);
        AbstractResource* resourceForFile(const QUrl &/*url*/);

    public Q_SLOTS:
        void installApplication(AbstractResource* app, const AddonList& addons);
        void installApplication(AbstractResource* app);
        void removeApplication(AbstractResource* app);

    Q_SIGNALS:
        void fetchingChanged();
        void allInitialized();
        void backendsChanged();
        void updatesCountChanged();
        void backendDataChanged(AbstractResourcesBackend* backend, const QVector<QByteArray>& properties);
        void resourceDataChanged(AbstractResource* resource, const QVector<QByteArray>& properties);
        void resourceRemoved(AbstractResource* resource);
        void passiveMessage(const QString &message);

    private Q_SLOTS:
        void callerFetchingChanged();
        void updateCaller(const QVector<QByteArray>& properties);
        void registerAllBackends();

    private:
        QVector<int> propertiesToRoles(const QVector<QByteArray>& propertyNames) const;

        ///@p initialize tells if all backends load will be triggered on construction
        explicit ResourcesModel(QObject* parent=nullptr, bool load = true);
        void init(bool load);
        void addResourcesBackend(AbstractResourcesBackend* backend);
        void registerBackendByName(const QString& name);

        QVector< AbstractResourcesBackend* > m_backends;
        int m_initializingBackends;
        KActionCollection* m_actionCollection;

        static ResourcesModel* s_self;
};

#endif // RESOURCESMODEL_H
