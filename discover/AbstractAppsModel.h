/*
 *   SPDX-FileCopyrightText: 2016-2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "resources/AbstractResourcesBackend.h"
#include <QAbstractListModel>

class AbstractAppsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY appsCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
    Q_PROPERTY(AbstractResourcesBackend *currentApplicationBackend READ currentApplicationBackend NOTIFY currentApplicationBackendChanged)
public:
    AbstractAppsModel();

    void setResources(const QList<StreamResult> &resources);
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    AbstractResourcesBackend *currentApplicationBackend() const
    {
        return m_backend;
    }

    bool isFetching() const
    {
        return m_isFetching != 0;
    }

    virtual void refresh() = 0;
    int count() const
    {
        return rowCount({});
    }

Q_SIGNALS:
    void appsCountChanged();
    void isFetchingChanged();
    void currentApplicationBackendChanged(AbstractResourcesBackend *currentApplicationBackend);

protected:
    void refreshCurrentApplicationBackend();
    void setUris(const QList<QUrl> &uris);
    void removeResource(AbstractResource *resource);

    void acquireFetching(bool f);

private:
    QList<StreamResult> m_resources;
    int m_isFetching = 0;
    AbstractResourcesBackend *m_backend = nullptr;
    QList<QUrl> m_uris;
};
