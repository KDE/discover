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

#include "ResourcesModel.h"

#include "resources/AbstractResourcesBackend.h"
#include "AbstractResource.h"
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <QDebug>

ResourcesModel::ResourcesModel(QObject* parent)
    : QAbstractListModel(parent)
{
    QHash< int, QByteArray > roles = roleNames();
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[CommentRole] = "comment";
    roles[ActionRole] = "action";
    roles[StatusRole] = "status";
    roles[RatingRole] = "rating";
    roles[RatingPointsRole] = "ratingPoints";
    roles[SortableRatingRole] = "sortableRating";
    roles[ActiveRole] = "active";
    roles[ProgressRole] = "progress";
    roles[ProgressTextRole] = "progressText";
    roles[InstalledRole] = "installed";
    roles[ApplicationRole] = "application";
    roles[UsageCountRole] = "usageCount";
    roles[PopConRole] = "popcon";
    roles[OriginRole] = "origin";
    roles[UntranslatedNameRole] = "untranslatedName";
    roles[CanUpgrade] = "canUpgrade";
    roles[PackageNameRole] = "packageName";
    roles[IsTechnicalRole] = "isTechnical";
    roles[CategoryRole] = "category";
    roles[SectionRole] = "section";
    setRoleNames(roles);
}

void ResourcesModel::addResourcesBackend(AbstractResourcesBackend* resources)
{
    QVector<AbstractResource*> newResources = resources->allResources();
    int current = rowCount();
    beginInsertRows(QModelIndex(), current, current+newResources.size());
    m_backends += resources;
    m_resources.append(newResources);
    endInsertRows();
    
    connect(resources, SIGNAL(backendReady()), SLOT(resetCaller()));
    connect(resources, SIGNAL(reloadStarted()), SLOT(cleanCaller()));
    connect(resources, SIGNAL(reloadFinished()), SLOT(resetCaller()));
    connect(resources, SIGNAL(updatesCountChanged()), SIGNAL(updatesCountChanged()));
}

AbstractResource* ResourcesModel::resourceAt(int row) const
{
    for(QVector<QVector<AbstractResource*> >::const_iterator it=m_resources.constBegin(); it!=m_resources.constEnd(); ++it) {
        if(it->size()<row)
            row -= it->size();
        else
            return it->at(row);
    }
    return 0;
}

QVariant ResourcesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    AbstractResource* resource = resourceAt(index.row());
    switch(role) {
        case ApplicationRole:
            return qVariantFromValue<QObject*>(resource);
        case RatingPointsRole:
        case RatingRole:
        case SortableRatingRole:{
            Rating* rating = backendForResource(resource)->reviewsBackend()->ratingForApplication(resource);
            return rating ? rating->property(roleNames().value(role)) : -1;
        }
        default:
            return resource->property(roleNames().value(role));
    }
}

int ResourcesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) return 0;
    int ret = 0;
    foreach(const QVector<AbstractResource*>& resources, m_resources) {
        ret += resources.size();
    }
    return ret;
}

void ResourcesModel::cleanCaller()
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());
    Q_ASSERT(backend);
    int pos = m_backends.indexOf(backend);
    Q_ASSERT(pos>=0);
    QVector<AbstractResource*>* backendsResources = &m_resources[pos];
    int before = 0;
    for(QVector<QVector<AbstractResource*> >::const_iterator it=m_resources.constBegin();
        it!=m_resources.constEnd() && it!=backendsResources;
        ++it)
    {
        before+= it->size();
    }
    
    beginRemoveRows(QModelIndex(), before, before+backendsResources->count());
    backendsResources->clear();
    endRemoveRows();
}

void ResourcesModel::resetCaller()
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());
    Q_ASSERT(backend);
    int pos = m_backends.indexOf(backend);
    Q_ASSERT(pos>=0);
    QVector<AbstractResource*>* backendsResources = &m_resources[pos];
    int before = 0;
    for(QVector<QVector<AbstractResource*> >::const_iterator it=m_resources.constBegin();
        it!=m_resources.constEnd() && it!=backendsResources;
        ++it)
    {
        before+= it->size();
    }
    
    QVector< AbstractResource* > res = backend->allResources();
    if(res.size()!=0) {
        beginInsertRows(QModelIndex(), before, before+backendsResources->count());
        *backendsResources = res;
        endInsertRows();
        emit updatesCountChanged();
    }
}

QVector< AbstractResourcesBackend* > ResourcesModel::backends() const
{
    return m_backends;
}

AbstractResource* ResourcesModel::applicationByPackageName(const QString& name)
{
    foreach(AbstractResourcesBackend* backend, m_backends)
    {
        AbstractResource* res = backend->resourceByPackageName(name);
        if(res) {
            return res;
        }
    }
    return 0;
}


AbstractResourcesBackend* ResourcesModel::backendForResource(AbstractResource* resource) const
{
    foreach(AbstractResourcesBackend* backend, m_backends)
    {
        if(backend->providesResouce(resource)) {
            return backend;
        }
    }
    Q_ASSERT(!resource && "all resources should have a backend");
    return 0;
}

int ResourcesModel::updatesCount() const
{
    int ret = 0;
    foreach(AbstractResourcesBackend* backend, m_backends) {
        ret += backend->updatesCount();
    }
    return ret;
}
