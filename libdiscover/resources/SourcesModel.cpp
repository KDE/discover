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
#include <QDebug>
#include <QAction>
#include "resources/AbstractResourcesBackend.h"
#include "resources/AbstractSourcesBackend.h"

Q_GLOBAL_STATIC(SourcesModel, s_sources)

class SourceBackendModel : public QAbstractListModel
{
Q_OBJECT
public:
    SourceBackendModel(AbstractResourcesBackend* backend, QObject* parent)
        : QAbstractListModel(parent), m_backend(backend)
    {}

    QVariant data(const QModelIndex & index, int role) const override {
        if (!index.isValid()) return {};
        switch(role) {
            case SourcesModel::ResourcesBackend: return QVariant::fromValue<QObject*>(m_backend);
            case SourcesModel::SourcesBackend: return QVariant::fromValue<QObject*>(m_sources);
        }
        return {};
    }
    int rowCount(const QModelIndex & parent) const override { return parent.isValid() ? 0 : 1; }

    AbstractSourcesBackend* m_sources = nullptr;

private:
    AbstractResourcesBackend* m_backend;
};

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
    roles.insert(SourcesBackend, "sourcesBackend");
    roles.insert(ResourcesBackend, "resourcesBackend");
    return roles;
}

static bool ensureModel(const QList<QByteArray> &roles)
{
    static auto required = {"display", "checked"};
    for (const auto &role: required) {
        if (!roles.contains(role))
            return false;
    }
    return true;
}

void SourcesModel::addSourcesBackend(AbstractSourcesBackend* sources)
{
    auto b = addBackend(qobject_cast<AbstractResourcesBackend*>(sources->parent()));
    if (!b)
        return;

    b->m_sources = sources;
    Q_ASSERT(ensureModel(sources->sources()->roleNames().values()));
    addSourceModel(sources->sources());
}

SourceBackendModel* SourcesModel::addBackend(AbstractResourcesBackend* backend)
{
    Q_ASSERT(backend);
    if (backend->dynamicPropertyNames().contains("InSourcesModel"))
        return nullptr;
    backend->setProperty("InSourcesModel", 1);

    auto b = new SourceBackendModel(backend, this);
    addSourceModel(b);
    return b;
}

#include "SourcesModel.moc"
