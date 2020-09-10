/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FWUPDSOURCESBACKEND_H
#define FWUPDSOURCESBACKEND_H

#include <resources/AbstractSourcesBackend.h>
#include "FwupdBackend.h"
#include <QStandardItemModel>

class FwupdSourcesModel;

class FwupdSourcesBackend : public AbstractSourcesBackend
{
    Q_OBJECT
public:
    explicit FwupdSourcesBackend(AbstractResourcesBackend * parent);

    FwupdBackend* backend ;
    QAbstractItemModel* sources() override;
    bool addSource(const QString& id) override;
    bool removeSource(const QString& id) override;
    QString idDescription() override { return QString(); }
    QVariantList actions() const override;
    bool supportsAdding() const override { return false; }
    void eulaRequired(const QString& remoteName, const QString& licenseAgreement);
    void populateSources();

    void proceed() override;
    void cancel() override;

    QStandardItem* m_currentItem = nullptr;

private:
    FwupdSourcesModel* m_sources;
};

#endif // FWUPDSOURCESBACKEND_H
