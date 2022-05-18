/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
#include <resources/ResourcesModel.h>
class AbstractResourcesBackend;

/// Display the apps at the top of the browsing page
class SpecialAppsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SpecialAppsModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    ~SpecialAppsModel() override = default;

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) {
            return {};
        }

        const auto appInfo = m_resources[index.row()];

        switch (role) {
        case Qt::UserRole: {
            if (!appInfo) {
                return {};
            }

            return QVariant::fromValue<QObject *>(appInfo);
        }
        }
        return {};
    }

    int rowCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return m_resources.count();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::UserRole, QByteArrayLiteral("applicationObject")}};
    }

    void setResources(const QVector<AbstractResource *> &resources)
    {
        if (!m_resources.isEmpty()) {
            beginRemoveRows({}, 0, m_resources.count() - 1);
            m_resources = {};
            endRemoveRows();
        }
        beginInsertRows({}, 0, resources.count() - 1);
        m_resources = resources;
        endInsertRows();
    }

private:
    QVector<AbstractResource *> m_resources;
};

struct FeaturedApp {
    QUrl id;
};

class FeaturedModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
    Q_PROPERTY(AbstractResourcesBackend *currentApplicationBackend READ currentApplicationBackend NOTIFY currentApplicationBackendChanged)
    Q_PROPERTY(QAbstractItemModel *specialApps READ specialApps CONSTANT)
public:
    explicit FeaturedModel(QObject *parent = nullptr);
    ~FeaturedModel() override = default;

    void setResources(const QVector<AbstractResource *> &resources);
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

    QVector<QPair<QString, QVector<AbstractResource *>>> m_resources;
    int m_isFetching = 0;
    AbstractResourcesBackend *m_backend = nullptr;
    SpecialAppsModel *m_specialAppsModel = nullptr;
};

#endif // FEATUREDMODEL_H
