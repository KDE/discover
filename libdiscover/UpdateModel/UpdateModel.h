/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include "discovercommon_export.h"
#include "resources/AbstractBackendUpdater.h"
#include <QAbstractListModel>

class QTimer;
class ResourcesUpdatesModel;
class AbstractResource;
class UpdateItem;

class DISCOVERCOMMON_EXPORT UpdateModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ResourcesUpdatesModel *backend READ backend WRITE setBackend)
    Q_PROPERTY(bool hasUpdates READ hasUpdates NOTIFY hasUpdatesChanged)
    Q_PROPERTY(int toUpdateCount READ toUpdateCount NOTIFY toUpdateChanged)
    Q_PROPERTY(int totalUpdatesCount READ totalUpdatesCount NOTIFY hasUpdatesChanged)
    Q_PROPERTY(QString updateSize READ updateSize NOTIFY updateSizeChanged)
public:
    enum Roles {
        SizeRole = Qt::UserRole + 1,
        ResourceRole,
        ResourceProgressRole,
        ResourceStateRole,
        SectionResourceProgressRole,
        ChangelogRole,
        SectionRole,
        UpgradeTextRole,
        ExtendedRole,
    };
    Q_ENUM(Roles)

    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    void setResources(const QList<AbstractResource *> &res);
    UpdateItem *itemFromIndex(const QModelIndex &index) const;

    void checkResources(const QList<AbstractResource *> &resource, bool checked);
    QHash<int, QByteArray> roleNames() const override;

    bool hasUpdates() const;

    /// all upgradeable packages
    int totalUpdatesCount() const;

    /// packages marked to upgrade
    int toUpdateCount() const;

    Q_SCRIPTABLE void fetchUpdateDetails(int row);

    QString updateSize() const;

    ResourcesUpdatesModel *backend() const;

public Q_SLOTS:
    void checkAll();
    void uncheckAll();

    void setBackend(ResourcesUpdatesModel *updates);

Q_SIGNALS:
    void hasUpdatesChanged(bool hasUpdates);
    void toUpdateChanged();
    void updateSizeChanged();

private:
    void resourceDataChanged(AbstractResource *res, const QVector<QByteArray> &properties);
    void integrateChangelog(const QString &changelog);
    QModelIndex indexFromItem(UpdateItem *item) const;
    UpdateItem *itemFromResource(AbstractResource *res);
    void resourceHasProgressed(AbstractResource *res, qreal progress, AbstractBackendUpdater::State state);
    void activityChanged();

    QTimer *const m_updateSizeTimer;
    QVector<UpdateItem *> m_updateItems;
    ResourcesUpdatesModel *m_updates;
    QList<AbstractResource *> m_resources;
};

#endif // UPDATEMODEL_H
