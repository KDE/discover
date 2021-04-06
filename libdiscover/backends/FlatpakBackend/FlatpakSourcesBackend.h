/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKSOURCESBACKEND_H
#define FLATPAKSOURCESBACKEND_H

#include <resources/AbstractSourcesBackend.h>
#include <QStandardItemModel>
#include <QStack>
#include <functional>

#include "flatpak-helper.h"

class FlatpakResource;
class FlatpakSourcesBackend : public AbstractSourcesBackend
{
Q_OBJECT
public:
    explicit FlatpakSourcesBackend(const QVector<FlatpakInstallation *>& installations, AbstractResourcesBackend *parent);
    ~FlatpakSourcesBackend() override;

    enum Roles { IconUrlRole = LastRole + 1 };

    QAbstractItemModel* sources() override;
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;
    QString idDescription() override;
    QVariantList actions() const override;
    bool supportsAdding() const override { return true; }
    bool canFilterSources() const override { return true; }

    FlatpakRemote * installSource(FlatpakResource *resource);
    bool canMoveSources() const override { return true; }

    bool moveSource(const QString & sourceId, int delta) override;
    int originIndex(const QString& sourceId) const;
    QStandardItem* sourceByUrl(const QString & url) const;
    QStandardItem* sourceById(const QString & sourceId) const;

    void cancel() override;
    void proceed() override;

private:
    bool listRepositories(FlatpakInstallation *installation);
    void addRemote(FlatpakRemote *remote, FlatpakInstallation *installation);

    FlatpakInstallation *m_preferredInstallation;
    QStandardItemModel* m_sources;
    QAction* const m_flathubAction;
    QStandardItem* m_noSourcesItem;
    QStack<std::function<void()>> m_proceedFunctions;
};

#endif // FLATPAKSOURCESBACKEND_H
