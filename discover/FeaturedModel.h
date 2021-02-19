/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicnseRef-KDE-Accepted-GPL
 */

#ifndef FEATUREDMODEL_H
#define FEATUREDMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QUrl>

namespace KIO
{
class StoredTransferJob;
}
class AbstractResource;
class AbstractResourcesBackend;
class SpecialAppsModel;

struct FeaturedApp {
    QUrl id;
    QString color;
    QString gradientStart;
    QString gradientEnd;
};

/// Displays the apps
class FeaturedModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
    Q_PROPERTY(AbstractResourcesBackend *currentApplicationBackend READ currentApplicationBackend NOTIFY currentApplicationBackendChanged)
    Q_PROPERTY(QAbstractItemModel *specialApps READ specialApps CONSTANT)
public:
    explicit FeaturedModel(QObject *parent = nullptr);
    ~FeaturedModel() override = default;

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
    AbstractResourcesBackend *currentApplicationBackend() const
    {
        return m_backend;
    }

    bool isFetching() const
    {
        return m_isFetching != 0;
    }

    QAbstractItemModel *specialApps() const;

Q_SIGNALS:
    void isFetchingChanged();
    void currentApplicationBackendChanged(AbstractResourcesBackend *currentApplicationBackend);

private:
    void setResources(const QString &category, const QVector<AbstractResource *> &resources);
    void refreshCurrentApplicationBackend();
    void setUris(const QHash<QString, QVector<QUrl>> &uris, const QVector<FeaturedApp> &featuredApp);
    void refresh();
    void removeResource(AbstractResource *resource);

    void acquireFetching(bool f);

    // we are using a vector to have
    QVector<QPair<QString, QVector<AbstractResource *>>> m_resources;
    int m_isFetching = 0;
    AbstractResourcesBackend* m_backend = nullptr;
    SpecialAppsModel *m_specialAppsModel;
};

#endif // FEATUREDMODEL_H
