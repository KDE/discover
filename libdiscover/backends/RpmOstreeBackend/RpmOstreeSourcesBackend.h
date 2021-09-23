/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RPMOSTREESOURCESBACKEND_H
#define RPMOSTREESOURCESBACKEND_H

#include <resources/SourcesModel.h>

#include <QStandardItemModel>

class RpmOstreeSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit RpmOstreeSourcesBackend(AbstractResourcesBackend *parent);
    QAbstractItemModel *sources() override;
    bool addSource(const QString &) override;
    bool removeSource(const QString &) override;
    QString idDescription() override;
    QVariantList actions() const override;
    bool supportsAdding() const override;
    bool canMoveSources() const override;

private:
    QStandardItemModel *const m_model;

    QHash<QString, QString> m_remotes;
};

#endif // RPMOSTREESOURCESBACKEND_H
