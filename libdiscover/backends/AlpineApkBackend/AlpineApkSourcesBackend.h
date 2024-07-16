/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ALPINEAPKSOURCESBACKEND_H
#define ALPINEAPKSOURCESBACKEND_H

#include "resources/AbstractSourcesBackend.h"

#include <QStandardItemModel>

#include <QtApkRepository.h>

class DiscoverAction;

class AlpineApkSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit AlpineApkSourcesBackend(AbstractResourcesBackend *parent);

    QAbstractItemModel *sources() override;
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;
    QString idDescription() override;
    QVariantList actions() const override;
    bool supportsAdding() const override;
    bool canMoveSources() const override;
    bool moveSource(const QString &sourceId, int delta) override;

private:
    QStandardItem *sourceForId(const QString &id) const;
    bool addSourceFull(const QString &id, const QString &comment, bool enabled);
    void loadSources();
    void saveSources();
    void fillModelFromRepos();
    void onItemChanged(QStandardItem *item);

    QStandardItemModel *m_sourcesModel = nullptr;
    DiscoverAction *m_refreshAction = nullptr;
    DiscoverAction *m_saveAction = nullptr;
    QVector<QtApk::Repository> m_repos;
};

#endif // ALPINEAPKSOURCESBACKEND_H
