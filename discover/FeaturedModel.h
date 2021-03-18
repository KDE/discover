/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FEATUREDMODEL_H
#define FEATUREDMODEL_H

#include <QAbstractListModel>
#include <QPointer>

namespace KIO
{
class StoredTransferJob;
}
class AbstractResource;
class AbstractResourcesBackend;

class FeaturedModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
public:
    FeaturedModel();
    ~FeaturedModel() override
    {
    }

    void setResources(const QVector<AbstractResource *> &resources);
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isFetching() const
    {
        return m_isFetching != 0;
    }

Q_SIGNALS:
    void isFetchingChanged();

private:
    void refreshCurrentApplicationBackend();
    void setUris(const QVector<QUrl> &uris);
    void refresh();
    void removeResource(AbstractResource *resource);

    void acquireFetching(bool f);

    QVector<AbstractResource *> m_resources;
    int m_isFetching = 0;
    AbstractResourcesBackend *m_backend = nullptr;
};

#endif // FEATUREDMODEL_H
