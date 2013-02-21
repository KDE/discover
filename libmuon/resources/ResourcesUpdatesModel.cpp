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
#include "AbstractResource.h"
#include <QDebug>
#include <QDateTime>
#include <KLocalizedString>
#include <KGlobal>
#include <KLocale>
#include <KDebug>

ResourcesUpdatesModel::ResourcesUpdatesModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_resources(0)
{
    setResourcesModel(ResourcesModel::global());
}

void ResourcesUpdatesModel::setResourcesModel(ResourcesModel* model)
{
    Q_ASSERT(model);
    m_resources = model;
    m_updaters.clear();
    addNewBackends();
    connect(model, SIGNAL(backendsChanged()), SLOT(addNewBackends()));
}

void ResourcesUpdatesModel::addNewBackends()
{
    QVector<AbstractResourcesBackend*> backends = ResourcesModel::global()->backends();
    foreach(AbstractResourcesBackend* b, backends) {
        AbstractBackendUpdater* updater = b->backendUpdater();
        if(updater && !m_updaters.contains(updater)) {
            connect(updater, SIGNAL(progressChanged(qreal)), SIGNAL(progressChanged()));
            connect(updater, SIGNAL(statusMessageChanged(QString)), SIGNAL(statusMessageChanged(QString)));
            connect(updater, SIGNAL(statusDetailChanged(QString)), SLOT(message(QString)));
            connect(updater, SIGNAL(remainingTimeChanged()), SIGNAL(etaChanged()));
            connect(updater, SIGNAL(downloadSpeedChanged(quint64)), SIGNAL(downloadSpeedChanged()));
            connect(updater, SIGNAL(progressingChanged(bool)), SIGNAL(progressingChanged()));
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

void ResourcesUpdatesModel::message(const QString& msg)
{
    appendRow(new QStandardItem(msg));
    emit statusDetailChanged(msg);
}

void ResourcesUpdatesModel::prepare()
{
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        upd->prepare();
    }
}

void ResourcesUpdatesModel::updateAll()
{
    Q_ASSERT(m_resources);
    m_finishedUpdaters = 0;
    
    if(m_updaters.isEmpty())
        emit progressingChanged();
    else foreach(AbstractBackendUpdater* upd, m_updaters) {
        QMetaObject::invokeMethod(upd, "start", Qt::QueuedConnection);
    }
}

void ResourcesUpdatesModel::updaterFinished()
{
    m_finishedUpdaters++;
    if(m_finishedUpdaters==m_updaters.size())
        emit progressingChanged();
}

QString ResourcesUpdatesModel::remainingTime() const
{
    long unsigned int maxEta = 0;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        maxEta = qMax(maxEta, upd->remainingTime());
    }

    // Ignore ETA if it's larger than 2 days.
    if(maxEta==0 && maxEta < 2 * 24 * 60 * 60)
        return QString();
    else
        return i18nc("@item:intext Remaining time", "%1 remaining",
                                KGlobal::locale()->prettyFormatDuration(maxEta));
}

QList<QAction*> ResourcesUpdatesModel::messageActions() const
{
    QList<QAction*> ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->messageActions();
    }
    return ret;
}

bool ResourcesUpdatesModel::hasUpdates() const
{
    bool ret = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret |= upd->hasUpdates();
    }
    return ret;
}

quint64 ResourcesUpdatesModel::downloadSpeed() const
{
    quint64 ret = 0;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->downloadSpeed();
    }
    return ret;
}

bool ResourcesUpdatesModel::isCancelable() const
{
    bool cancelable = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        cancelable |= upd->isCancelable();
    }
    return cancelable;
}

bool ResourcesUpdatesModel::isProgressing() const
{
    bool progressing = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        progressing |= upd->isProgressing();
    }
    return progressing;
}

bool ResourcesUpdatesModel::isAllMarked() const
{
    bool allmarked = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        allmarked |= upd->isAllMarked();
    }
    return allmarked;
}

QList<AbstractResource*> ResourcesUpdatesModel::toUpdate() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->toUpdate();
    }
    return ret;
}

void ResourcesUpdatesModel::addResources(const QList<AbstractResource*>& resources)
{
    QMap<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->addResources(*it);
    }
}

void ResourcesUpdatesModel::removeResources(const QList< AbstractResource* >& resources)
{
    QMap<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->removeResources(*it);
    }
}

QDateTime ResourcesUpdatesModel::lastUpdate() const
{
    QDateTime ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        QDateTime current = upd->lastUpdate();
        if(!ret.isValid() || (current.isValid() && current>ret)) {
            ret = current;
        }
    }
    return ret;
}

void ResourcesUpdatesModel::cancel()
{
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        if(upd->isCancelable())
            upd->cancel();
        else
            kWarning() << "tried to cancel " << upd->metaObject()->className() << "which is not cancelable";
    }
}
