/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include "AbstractSourcesBackend.h"
#include "discovercommon_export.h"
#include <KConcatenateRowsProxyModel>
#include <QAbstractListModel>
#include <QSet>

class DISCOVERCOMMON_EXPORT SourcesModel : public KConcatenateRowsProxyModel
{
    Q_OBJECT
public:
    enum Roles {
        SourceNameRole = AbstractSourcesBackend::LastRole,
        SourcesBackend,
        ResourcesBackend,
        EnabledRole,
    };
    Q_ENUM(Roles)

    explicit SourcesModel(QObject *parent = nullptr);
    ~SourcesModel() override;

    static SourcesModel *global();
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addSourcesBackend(AbstractSourcesBackend *sources);

    Q_SCRIPTABLE AbstractSourcesBackend *sourcesBackendByName(const QString &name) const;

Q_SIGNALS:
    void showingNow();

private:
    const QAbstractItemModel *modelAt(const QModelIndex &idx) const;
};

#endif // SOURCESMODEL_H
