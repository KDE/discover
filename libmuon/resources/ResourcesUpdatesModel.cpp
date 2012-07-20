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

#include "ResourcesUpdatesModel.h"
#include "ResourcesModel.h"
#include "AbstractBackendUpdater.h"
#include <QDebug>

ResourcesUpdatesModel::ResourcesUpdatesModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_resources(0)
{
}

ResourcesModel* ResourcesUpdatesModel::resourcesModel() const
{
    return m_resources;
}

void ResourcesUpdatesModel::setResourcesModel(ResourcesModel* model)
{
    Q_ASSERT(model);
    m_resources = model;
    m_updaters.clear();
    QVector< AbstractResourcesBackend* > backends = model->backends();
    foreach(AbstractResourcesBackend* b, backends) {
        AbstractBackendUpdater* updater = b->backendUpdater();
        if(updater && updater->hasUpdates()) {
            connect(updater, SIGNAL(progressChanged(qreal)), SIGNAL(progressChanged()));
            connect(updater, SIGNAL(message(QIcon,QString)), SLOT(message(QIcon,QString)));
            connect(updater, SIGNAL(updatesFinnished()), SLOT(updaterFinished()));
            m_updaters += updater;
        }
    }
}

qreal ResourcesUpdatesModel::progress() const
{
    qreal total = 0;
    foreach(AbstractBackendUpdater* updater, m_updaters) {
        total += updater->progress();
    }
    return total / m_updaters.count();
}

void ResourcesUpdatesModel::message(const QIcon& icon, const QString& msg)
{
    QStandardItem* item = new QStandardItem(icon, msg);
    appendRow(item);
}

void ResourcesUpdatesModel::updateAll()
{
    Q_ASSERT(m_resources);
    m_finishedUpdaters = 0;
    
    if(m_updaters.isEmpty())
        emit updatesFinnished();
    else foreach(AbstractBackendUpdater* upd, m_updaters)
        upd->start();
}

void ResourcesUpdatesModel::updaterFinished()
{
    m_finishedUpdaters++;
    if(m_finishedUpdaters==m_updaters.size())
        emit updatesFinnished();
}
