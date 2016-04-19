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

#include "UpdateModel.h"

// Qt includes
#include <QFont>
#include <QDebug>

// KDE includes
#include <KFormat>
#include <KLocalizedString>

// Own includes
#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/ResourcesModel.h>

UpdateModel::UpdateModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_updates(nullptr)
{

    connect(ResourcesModel::global(), &ResourcesModel::fetchingChanged, this, &UpdateModel::activityChanged);
    connect(ResourcesModel::global(), &ResourcesModel::updatesCountChanged, this, &UpdateModel::activityChanged);
}

UpdateModel::~UpdateModel()
{
}

QHash<int,QByteArray> UpdateModel::roleNames() const
{
    return QAbstractItemModel::roleNames().unite({
        { Qt::CheckStateRole, "checked" },
        { ResourceProgressRole, "resourceProgress" },
        { ResourceRole, "resource" },
        { SizeRole, "size" },
        { VersionRole, "version" },
        { SectionRole, "section" },
        { ChangelogRole, "changelog" }
    } );
}

void UpdateModel::setBackend(ResourcesUpdatesModel* updates)
{
    if (m_updates) {
        disconnect(m_updates, nullptr, this, nullptr);
    }

    m_updates = updates;

    connect(m_updates, &ResourcesUpdatesModel::progressingChanged, this, &UpdateModel::activityChanged);
    connect(m_updates, &ResourcesUpdatesModel::resourceProgressed, this, &UpdateModel::resourceHasProgressed);

    activityChanged();
}

void UpdateModel::resourceHasProgressed(AbstractResource* res, qreal progress)
{
    UpdateItem* item = itemFromResource(res);
    Q_ASSERT(item);
    item->setProgress(progress);

    const QModelIndex idx = indexFromItem(item);
    Q_EMIT dataChanged(idx, idx, { ResourceProgressRole });
}

void UpdateModel::activityChanged()
{
    if(ResourcesModel::global()->isFetching()) {
        setResources(QList<AbstractResource*>());
    } else if(!m_updates->isProgressing()) {
        m_updates->prepare();
        setResources(m_updates->toUpdate());
    }
}

QVariant UpdateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    UpdateItem *item = itemFromIndex(index);

    switch (role) {
    case Qt::DisplayRole:
        return item->name();
    case Qt::DecorationRole:
        return item->icon();
    case Qt::CheckStateRole:
        return item->checked();
    case VersionRole:
        return item->version();
    case SizeRole:
        return KFormat().formatByteSize(item->size());
    case ResourceRole:
        return QVariant::fromValue<QObject*>(item->resource());
    case ResourceProgressRole:
        return item->progress();
    case ChangelogRole:
        return item->changelog();
    case SectionRole:
        return item->section();
    default:
        break;
    }

    return QVariant();
}

void UpdateModel::checkResources(const QList<AbstractResource*>& resource, bool checked)
{
    if(checked)
        m_updates->addResources(resource);
    else
        m_updates->removeResources(resource);
}

Qt::ItemFlags UpdateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int UpdateModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_updateItems.count() : 0;
}

bool UpdateModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        UpdateItem *item = itemFromIndex(idx);
        const bool newValue = value.toInt() == Qt::Checked;
        const QList<AbstractResource *> apps = { item->app() };

        checkResources(apps, newValue);
        Q_ASSERT(idx.data(Qt::CheckStateRole) == value);
        Q_EMIT dataChanged(idx, idx, { Qt::CheckStateRole });
        Q_EMIT toUpdateChanged();

        return true;
    }

    return false;
}

void UpdateModel::integrateChangelog(const QString &changelog)
{
    auto app = qobject_cast<AbstractResource*>(sender());
    Q_ASSERT(app);
    auto item = itemFromResource(app);
    if (!item)
        return;

    item->setChangelog(changelog);

    const QModelIndex idx = indexFromItem(item);
    Q_ASSERT(idx.isValid());
    emit dataChanged(idx, idx, { ChangelogRole });
}

void UpdateModel::setResources(const QList< AbstractResource* >& resources)
{
    beginResetModel();
    qDeleteAll(m_updateItems);
    m_updateItems.clear();

    const QString importantUpdatesSection = i18nc("@item:inlistbox", "Important Security Updates");
    const QString appUpdatesSection = i18nc("@item:inlistbox", "Application Updates");
    const QString systemUpdateSection = i18nc("@item:inlistbox", "System Updates");
    QVector<UpdateItem*> securityItems, appItems, systemItems;
    foreach(AbstractResource* res, resources) {
        connect(res, &AbstractResource::changelogFetched, this, &UpdateModel::integrateChangelog, Qt::UniqueConnection);

        UpdateItem *updateItem = new UpdateItem(res);

        if (res->isFromSecureOrigin()) {
            updateItem->setSection(importantUpdatesSection);
            securityItems += updateItem;
        } else if(!res->isTechnical()) {
            updateItem->setSection(appUpdatesSection);
            appItems += updateItem;
        } else {
            updateItem->setSection(systemUpdateSection);
            systemItems += updateItem;
        }
    }
    const auto sortUpdateItems = [](UpdateItem *a, UpdateItem *b) { return a->name() < b->name(); };
    qSort(securityItems.begin(), securityItems.end(), sortUpdateItems);
    qSort(appItems.begin(), appItems.end(), sortUpdateItems);
    qSort(systemItems.begin(), systemItems.end(), sortUpdateItems);
    m_updateItems = (QVector<UpdateItem*>() << securityItems << appItems << systemItems);
    endResetModel();

    foreach(AbstractResource* res, resources) {
        res->fetchChangelog();
    }

    Q_EMIT hasUpdatesChanged(!resources.isEmpty());
}

bool UpdateModel::hasUpdates() const
{
    return rowCount() > 0;
}

ResourcesUpdatesModel* UpdateModel::backend() const
{
    return m_updates;
}

int UpdateModel::toUpdateCount() const
{
    int ret = 0;
    foreach (UpdateItem* item, m_updateItems) {
        ret += item->checked();
    }
    return ret;
}

UpdateItem * UpdateModel::itemFromResource(AbstractResource* res)
{
    foreach (UpdateItem* item, m_updateItems) {
        if (item->app() == res)
            return item;
    }
    return nullptr;
}

QString UpdateModel::updateSize() const
{
    double ret = 0;
    foreach (UpdateItem* item, m_updateItems) {
        if (item->checked() == Qt::Checked)
            ret += item->size();
    }
    return KFormat().formatByteSize(ret);
}

QModelIndex UpdateModel::indexFromItem(UpdateItem* item) const
{
    return index(m_updateItems.indexOf(item), 0, {});
}

UpdateItem * UpdateModel::itemFromIndex(const QModelIndex& index) const
{
    return m_updateItems[index.row()];
}
