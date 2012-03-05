/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

#include "LaunchListModel.h"
#include <Application.h>
#include <ApplicationBackend.h>
#include <QStringBuilder>
#include <QDebug>
#include <KIcon>
#include <KService>
#include <KToolInvocation>

LaunchListModel::LaunchListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_backend(0)
{}

void LaunchListModel::setBackend(ApplicationBackend* backend)
{
    if(m_backend)
        disconnect(m_backend, SIGNAL(launchListChanged()), this, SLOT(resetApplications()));
    m_backend = backend;
    if(m_backend) {
        connect(m_backend, SIGNAL(launchListChanged()), this, SLOT(resetApplications()));
        resetApplications();
    }
}

void LaunchListModel::resetApplications()
{
    clear();
    QList<QStandardItem*> items;
    foreach (Application *app, m_backend->launchList()) {
        QVector<KService::Ptr> execs = app->executables();
        foreach (KService::Ptr service, execs) {
            QString name = service->genericName().isEmpty() ?
                        service->property("Name").toString() :
                        service->property("Name").toString() % QLatin1Literal(" - ") % service->genericName();
            QStandardItem *item = new QStandardItem(name);
            item->setIcon(KIcon(service->icon()));
            item->setData(service->desktopEntryPath(), Qt::UserRole);
            items += item;
        }
    }
    invisibleRootItem()->appendRows(items);
}

void LaunchListModel::invokeApplication(int row) const
{
    KToolInvocation::startServiceByDesktopPath(index(row, 0).data(Qt::UserRole).toString());
}
