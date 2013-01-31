/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <resources/StandardBackendUpdater.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractResource.h>
#include "ResourcesModel.h"
#include <Transaction/Transaction.h>
#include <KLocalizedString>
#include <KAction>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QIcon>

StandardBackendUpdater::StandardBackendUpdater(AbstractResourcesBackend* parent)
    : AbstractBackendUpdater(parent)
    , m_backend(parent)
    , m_preparedSize(0)
    , m_settingUp(false)
    , m_lastUpdate(QDateTime())
{
    connect(parent,
            SIGNAL(transactionRemoved(Transaction*)),
            SLOT(transactionRemoved(Transaction*)));
    connect(this, SIGNAL(updatesFinnished()), SLOT(cleanup()));
}

bool StandardBackendUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

void StandardBackendUpdater::start()
{
    emit progressingChanged(true);
    m_settingUp = true;
    setProgress(-1);
    foreach(AbstractResource* res, m_toUpgrade) {
        m_pendingResources += res;
        m_backend->installApplication(res);
    }

    if(m_pendingResources.isEmpty()) {
        emit updatesFinnished();
        emit progressingChanged(false);
    }
    m_settingUp = false;
}

void StandardBackendUpdater::transactionRemoved(Transaction* t)
{
    bool found = m_pendingResources.remove(t->resource());
    if(found && !m_settingUp) {
        setStatusMessage(i18n("%1 has been updated", t->resource()->name()));
        qreal p = 1-(qreal(m_pendingResources.size())/m_toUpgrade.size());
        setProgress(100*p);
        if(m_pendingResources.isEmpty()) {
            emit updatesFinnished();
            emit progressingChanged(false);
        }
    }
}

qreal StandardBackendUpdater::progress() const
{
    return m_progress;
}

void StandardBackendUpdater::setProgress(qreal p)
{
    if(p>m_progress || p<0) {
        m_progress = p;
        emit progressChanged(p);
    }
}

long unsigned int StandardBackendUpdater::remainingTime() const
{
    return 0;
}

void StandardBackendUpdater::prepare()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade = m_backend->upgradeablePackages().toSet();
    m_preparedSize = m_toUpgrade.size();
}

void StandardBackendUpdater::addResources(const QList< AbstractResource* >& apps)
{
    m_toUpgrade += apps.toSet();
}

void StandardBackendUpdater::removeResources(const QList< AbstractResource* >& apps)
{
    m_toUpgrade -= apps.toSet();
}

void StandardBackendUpdater::cleanup()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade.clear();
}

QList<AbstractResource*> StandardBackendUpdater::toUpdate() const
{
    return m_toUpgrade.toList();
}

bool StandardBackendUpdater::isAllMarked() const
{
    //Maybe we should make this smarter...
    return m_preparedSize>=m_toUpgrade.size();
}

QDateTime StandardBackendUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

bool StandardBackendUpdater::isCancelable() const
{
    //We don't really know when we can cancel, so we never let
    return false;
}

bool StandardBackendUpdater::isProgressing() const
{
    return m_settingUp || !m_pendingResources.isEmpty();
}

QString StandardBackendUpdater::statusDetail() const
{
    return QString();
}

void StandardBackendUpdater::setStatusMessage(const QString& msg)
{
    m_statusMessage = msg;
    emit message(QIcon(), msg);
    emit statusMessageChanged(msg);
}

QString StandardBackendUpdater::statusMessage() const
{
    return m_statusMessage;
}

quint64 StandardBackendUpdater::downloadSpeed() const
{
    return 0;
}

QList<QAction*> StandardBackendUpdater::messageActions() const
{
    return QList<QAction*>();
}
