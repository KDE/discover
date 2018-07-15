/***************************************************************************
*   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#include <FwupdUpdater.h>

#include <QDebug>
#include <QSet>

FwupdUpdater::FwupdUpdater(FwupdBackend * parent)
    : AbstractBackendUpdater(parent),
      m_transaction(nullptr),
      m_backend(parent),
      m_isCancelable(false),
      m_isProgressing(false),
      m_percentage(0),
      m_lastUpdate()
{
}

FwupdUpdater::~FwupdUpdater()
{

}

void FwupdUpdater::prepare()
{
    Q_ASSERT(!m_transaction);
    m_toUpgrade = m_backend->FwupdGetAllUpdates();
    m_allUpgradeable = m_toUpgrade;
}

int FwupdUpdater::updatesCount()
{
    return m_toUpgrade.count();
}

void FwupdUpdater::start()
{
    Q_ASSERT(!isProgressing());
}

void FwupdUpdater::cancel()
{
    if (m_transaction)
        m_transaction->cancel();
    else
        setProgressing(false);
}

void FwupdUpdater::setProgressing(bool progressing)
{
    if (m_isProgressing != progressing) 
    {
        m_isProgressing = progressing;
        emit progressingChanged(m_isProgressing);
    }
}

quint64 FwupdUpdater::downloadSpeed() const
{
    return m_transaction ? m_transaction->speed() : 0;
}

double FwupdUpdater::updateSize() const
{
    double ret = 0.;
    QSet<QString> donePkgs;
    for (AbstractResource * res : m_toUpgrade)
    {
        FwupdResource * app = qobject_cast<FwupdResource*>(res);
        QString pkgid = app->m_id;
        if (!donePkgs.contains(pkgid)) 
        {
            donePkgs.insert(pkgid);
            ret += app->size();
        }
    }
    return ret;
}

bool FwupdUpdater::isMarked(AbstractResource* res) const
{
    return m_toUpgrade.contains(res);
}

bool FwupdUpdater::isProgressing() const
{
    return m_isProgressing;
}

bool FwupdUpdater::isCancelable() const
{
    return m_isCancelable;
}

QDateTime FwupdUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

QList<AbstractResource*> FwupdUpdater::toUpdate() const
{
    return m_toUpgrade.toList();
}

void FwupdUpdater::addResources(const QList<AbstractResource*>& apps)
{
    QSet<QString> pkgs = involvedResources(apps.toSet());
    m_toUpgrade.unite(resourcesForResourceId(pkgs));
}

void FwupdUpdater::removeResources(const QList<AbstractResource*>& apps)
{
    QSet<QString> pkgs = involvedResources(apps.toSet());
    m_toUpgrade.subtract(resourcesForResourceId(pkgs));
}

QSet<QString> FwupdUpdater::involvedResources(const QSet<AbstractResource*>& resources) const
{
    QSet<QString> resIds;
    resIds.reserve(resources.size());
    foreach (AbstractResource * res, resources)
    {
        FwupdResource * app = qobject_cast<FwupdResource*>(res);
        QString resid = app->m_id;
        resIds.insert(resid);
    }
    return resIds;
}

QSet<AbstractResource*> FwupdUpdater::resourcesForResourceId(const QSet<QString>& resids) const
{
    QSet<QString> resources;
    resources.reserve(resids.size());
    foreach(const QString& resid, resids) {
        resources += m_backend->FwupdGetAppName(resid);
    }

    QSet<AbstractResource*> ret;
    foreach (AbstractResource * res, m_allUpgradeable)
    {
        FwupdResource* pres = qobject_cast<FwupdResource*>(res);
        if (resources.contains(pres->allResourceNames().toSet()))
        {
            ret.insert(res);
        }
    }

    return ret;
}

qreal FwupdUpdater::progress() const
{
    return m_percentage;
}

bool FwupdUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

