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

#include "DummySourcesBackend.h"
#include <QDebug>
#include <QAction>

DummySourcesBackend::DummySourcesBackend(AbstractResourcesBackend * parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new QStandardItemModel(this))
    , m_testAction(new QAction(QIcon::fromTheme(QStringLiteral("kalgebra")), QStringLiteral("DummyAction"), this))
{
    for (int i = 0; i<10; ++i)
        addSource(QStringLiteral("DummySource%1").arg(i));

    connect(m_testAction, &QAction::triggered, [](){ qDebug() << "action triggered!"; });
    connect(m_sources, &QStandardItemModel::itemChanged, this, [](QStandardItem* item) { qDebug() << "DummySource changed" << item << item->checkState(); });
}

QAbstractItemModel* DummySourcesBackend::sources()
{
    return m_sources;
}

bool DummySourcesBackend::addSource(const QString& id)
{
    if (id.isEmpty())
        return false;

    QStandardItem* it = new QStandardItem(id);
    it->setData(id, AbstractSourcesBackend::IdRole);
    it->setData(QVariant(id + QLatin1Char(' ') + id), Qt::ToolTipRole);
    it->setCheckable(true);
    it->setCheckState(Qt::Checked);
    m_sources->appendRow(it);
    return true;
}

QStandardItem * DummySourcesBackend::sourceForId(const QString& id) const
{
    for (int i=0, c=m_sources->rowCount(); i<c; ++i) {
        const auto it = m_sources->item(i, 0);
        if (it->text() == id)
            return it;
    }
    return nullptr;
}

bool DummySourcesBackend::removeSource(const QString& id)
{
    const auto it = sourceForId(id);
    if (!it) {
        qWarning() << "couldn't find " << id;
        return false;
    }
    return m_sources->removeRow(it->row());
}

QVariantList DummySourcesBackend::actions() const
{
    return QVariantList() << QVariant::fromValue<QObject*>(m_testAction);
}

bool DummySourcesBackend::moveSource(const QString& sourceId, int delta)
{
    int row = sourceForId(sourceId)->row();
    auto prevRow = m_sources->takeRow(row);
    Q_ASSERT(!prevRow.isEmpty());

    const auto destRow = row + delta;
    m_sources->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0)
        Q_EMIT firstSourceIdChanged();
    if (destRow == m_sources->rowCount() - 1 || row == m_sources->rowCount() - 1)
        Q_EMIT lastSourceIdChanged();
    return true;
}
