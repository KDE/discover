/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QAbstractListModel>
#include "discovercommon_export.h"

class ResourcesUpdatesModel;
class AbstractResource;
class UpdateItem;

class DISCOVERCOMMON_EXPORT UpdateModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ResourcesUpdatesModel* backend READ backend WRITE setBackend)
    Q_PROPERTY(bool hasUpdates READ hasUpdates NOTIFY hasUpdatesChanged)
    Q_PROPERTY(int toUpdateCount READ toUpdateCount NOTIFY toUpdateChanged)
    Q_PROPERTY(int totalUpdatesCount READ totalUpdatesCount NOTIFY hasUpdatesChanged)
    Q_PROPERTY(QString updateSize READ updateSize NOTIFY toUpdateChanged)
public:

    enum Roles {
        VersionRole = Qt::UserRole + 1,
        SizeRole,
        ResourceRole,
        ResourceProgressRole,
        ChangelogRole,
        SectionRole
    };

    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    void setResources(const QList<AbstractResource*>& res);
    UpdateItem *itemFromIndex(const QModelIndex &index) const;

    void checkResources(const QList<AbstractResource*>& resource, bool checked);
    QHash<int,QByteArray> roleNames() const override;

    bool hasUpdates() const;

    ///all upgradeable packages
    int totalUpdatesCount() const { return m_updateItems.count(); }

    ///packages marked to upgrade
    int toUpdateCount() const;

    Q_SCRIPTABLE void fetchChangelog(int row);

    QString updateSize() const;

    ResourcesUpdatesModel* backend() const;

public Q_SLOTS:
    void setBackend(ResourcesUpdatesModel* updates);

Q_SIGNALS:
    void hasUpdatesChanged(bool hasUpdates);
    void toUpdateChanged();

private:
    void integrateChangelog(const QString &changelog);
    QModelIndex indexFromItem(UpdateItem* item) const;
    UpdateItem* itemFromResource(AbstractResource* res);
    void resourceHasProgressed(AbstractResource* res, qreal progress);
    void activityChanged();

    QVector<UpdateItem*> m_updateItems;
    ResourcesUpdatesModel* m_updates;
};

#endif // UPDATEMODEL_H
