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

#include "SourcesModel.h"
#include <QtGlobal>
#include "libdiscover_debug.h"
#include <QAction>
#include "resources/AbstractResourcesBackend.h"
#include "resources/AbstractSourcesBackend.h"

Q_GLOBAL_STATIC(SourcesModel, s_sources)

const auto DisplayName = "DisplayName";
const auto SourcesBackendId = "SourcesBackend";

SourcesModel::SourcesModel(QObject* parent)
    : KConcatenateRowsProxyModel(parent)
{}

SourcesModel::~SourcesModel() = default;

SourcesModel* SourcesModel::global()
{
    return s_sources;
}

QHash<int, QByteArray> SourcesModel::roleNames() const
{
    QHash<int, QByteArray> roles = KConcatenateRowsProxyModel::roleNames();
    roles.insert(AbstractSourcesBackend::IdRole, "sourceId");
    roles.insert(SourceNameRole, "sourceName");
    roles.insert(SourcesBackend, "sourcesBackend");
    roles.insert(ResourcesBackend, "resourcesBackend");
    roles.insert(EnabledRole, "enabled");
    return roles;
}

void SourcesModel::addSourcesBackend(AbstractSourcesBackend* sources)
{
    auto backend = qobject_cast<AbstractResourcesBackend*>(sources->parent());

    auto m = sources->sources();
    m->setProperty(DisplayName, backend->displayName());
    m->setProperty(SourcesBackendId, qVariantFromValue<QObject*>(sources));
    addSourceModel(m);

    if (!m->rowCount())
        qWarning() << "adding empty sources model" << m;
}

const QAbstractItemModel * SourcesModel::modelAt(const QModelIndex& index) const
{
    const auto sidx = mapToSource(index);
    return sidx.model();
}

QVariant SourcesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    switch (role) {
        case SourceNameRole:
            return modelAt(index)->property(DisplayName);
        case SourcesBackend:
            return modelAt(index)->property(SourcesBackendId);
        case EnabledRole:
            return QVariant(flags(index) & Qt::ItemIsEnabled);
        default:
            return KConcatenateRowsProxyModel::data(index, role);
    }
}

AbstractSourcesBackend * SourcesModel::sourcesBackendByName(const QString& id) const
{
    for(int i = 0, c = rowCount(); i < c; ++i) {
        const auto idx = index(i, 0);
        if (idx.data(SourceNameRole) == id) {
            return qobject_cast<AbstractSourcesBackend *>(idx.data(SourcesBackend).value<QObject*>());
        }
    }
    return nullptr;
}
