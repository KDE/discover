/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SourcesModel.h"
#include "libdiscover_debug.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/AbstractSourcesBackend.h"
#include <QtGlobal>

Q_GLOBAL_STATIC(SourcesModel, s_sources)

const auto DisplayName = "DisplayName";
const auto SourcesBackendId = "SourcesBackend";

SourcesModel::SourcesModel(QObject *parent)
    : KConcatenateRowsProxyModel(parent)
{
}

SourcesModel::~SourcesModel() = default;

SourcesModel *SourcesModel::global()
{
    return s_sources;
}

QHash<int, QByteArray> SourcesModel::roleNames() const
{
    QHash<int, QByteArray> roles = KConcatenateRowsProxyModel::roleNames();
    roles.insert(AbstractSourcesBackend::IdRole, "sourceId");
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::ToolTipRole, "toolTip");
    roles.insert(SourceNameRole, "sourceName");
    roles.insert(SourcesBackend, "sourcesBackend");
    roles.insert(ResourcesBackend, "resourcesBackend");
    roles.insert(EnabledRole, "enabled");
    return roles;
}

void SourcesModel::addSourcesBackend(AbstractSourcesBackend *sources)
{
    auto backend = qobject_cast<AbstractResourcesBackend *>(sources->parent());

    auto m = sources->sources();
    m->setProperty(DisplayName, backend->displayName());
    m->setProperty(SourcesBackendId, QVariant::fromValue<QObject *>(sources));
    addSourceModel(m);

    if (!m->rowCount())
        qWarning() << "adding empty sources model" << m;
}

const QAbstractItemModel *SourcesModel::modelAt(const QModelIndex &index) const
{
    const auto sidx = mapToSource(index);
    return sidx.model();
}

QVariant SourcesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
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

AbstractSourcesBackend *SourcesModel::sourcesBackendByName(const QString &id) const
{
    for (int i = 0, c = rowCount(); i < c; ++i) {
        const auto idx = index(i, 0);
        if (idx.data(SourceNameRole) == id) {
            return qobject_cast<AbstractSourcesBackend *>(idx.data(SourcesBackend).value<QObject *>());
        }
    }
    return nullptr;
}
