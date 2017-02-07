/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "FlatpakSourcesBackend.h"
#include <QDebug>
#include <QAction>

FlatpakSourcesBackend::FlatpakSourcesBackend(QObject* parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new QStandardItemModel(this))
    , m_testAction(new QAction(QIcon::fromTheme(QStringLiteral("kalgebra")), QStringLiteral("FlatpakAction"), this))
{
    QHash<int, QByteArray> roles = m_sources->roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    m_sources->setItemRoleNames(roles);

    for (int i = 0; i<10; ++i)
        addSource(QStringLiteral("FlatpakSource%1").arg(i));

    connect(m_testAction, &QAction::triggered, [](){ qDebug() << "action triggered!"; });
}

QAbstractItemModel* FlatpakSourcesBackend::sources()
{
    return m_sources;
}

bool FlatpakSourcesBackend::addSource(const QString& id)
{
    QStandardItem* it = new QStandardItem(id);
    it->setData(QVariant(id + QLatin1Char(' ') + id), Qt::ToolTipRole);
    it->setData(name(), AbstractSourcesBackend::SectionRole);
    m_sources->appendRow(it);
    return true;
}

bool FlatpakSourcesBackend::removeSource(const QString& id)
{
    QList<QStandardItem*> items = m_sources->findItems(id);
    if (items.count()==1) {
        m_sources->removeRow(items.first()->row());
    } else {
        qWarning() << "couldn't find " << id  << items;
    }
    return items.count()==1;
}

QList<QAction*> FlatpakSourcesBackend::actions() const
{
    return QList<QAction*>() << m_testAction;
}

