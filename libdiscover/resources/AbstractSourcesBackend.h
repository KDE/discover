/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef ABSTRACTSOURCESMANAGER_H
#define ABSTRACTSOURCESMANAGER_H

#include <QObject>
#include "discovercommon_export.h"

class QAction;
class QAbstractItemModel;
class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT AbstractSourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractResourcesBackend* resourcesBackend READ resourcesBackend CONSTANT)
    Q_PROPERTY(QAbstractItemModel* sources READ sources CONSTANT)
    Q_PROPERTY(QString idDescription READ idDescription CONSTANT)
    Q_PROPERTY(QList<QAction*> actions READ actions CONSTANT)
    Q_PROPERTY(bool supportsAdding READ supportsAdding CONSTANT)
    Q_PROPERTY(bool canMoveSources READ canMoveSources CONSTANT)
    Q_PROPERTY(bool canFilterSources READ canFilterSources CONSTANT)
    Q_PROPERTY(QString firstSourceId READ firstSourceId NOTIFY firstSourceIdChanged)
    Q_PROPERTY(QString lastSourceId READ lastSourceId NOTIFY lastSourceIdChanged)
    public:
        explicit AbstractSourcesBackend(AbstractResourcesBackend* parent);
        ~AbstractSourcesBackend() override;

        enum Roles {
            IdRole = Qt::UserRole,
            LastRole
        };
        Q_ENUM(Roles)

        virtual QString idDescription() = 0;

        Q_SCRIPTABLE virtual bool addSource(const QString& id) = 0;
        Q_SCRIPTABLE virtual bool removeSource(const QString& id) = 0;

        virtual QAbstractItemModel* sources() = 0;
        virtual QList<QAction*> actions() const = 0;

        virtual bool supportsAdding() const = 0;

        AbstractResourcesBackend* resourcesBackend() const;

        virtual bool canFilterSources() const { return false; }
        virtual bool canMoveSources() const { return false; }
        Q_SCRIPTABLE virtual bool moveSource(const QString &sourceId, int delta);

        QString firstSourceId() const;
        QString lastSourceId() const;

    public Q_SLOTS:
        virtual void cancel() {}
        virtual void proceed() {}

    Q_SIGNALS:
        void firstSourceIdChanged();
        void lastSourceIdChanged();
        void passiveMessage(const QString &message);
        void proceedRequest(const QString &title, const QString &description);
};

#endif // ABSTRACTRESOURCESBACKEND_H
