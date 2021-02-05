/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DUMMYSOURCESBACKEND_H
#define DUMMYSOURCESBACKEND_H

#include <resources/AbstractSourcesBackend.h>
#include <QStandardItemModel>

class DiscoverAction;

class DummySourcesBackend : public AbstractSourcesBackend
{
public:
    explicit DummySourcesBackend(AbstractResourcesBackend * parent);

    QAbstractItemModel* sources() override;
    bool addSource(const QString& id) override;
    bool removeSource(const QString& id) override;
    QString idDescription() override { return QStringLiteral("Random weird text"); }
    QVariantList actions() const override;
    bool supportsAdding() const override { return true; }

    bool canMoveSources() const override { return true; }
    bool moveSource(const QString & sourceId, int delta) override;

private:
    QStandardItem* sourceForId(const QString& id) const;

    QStandardItemModel* m_sources;
    DiscoverAction* m_testAction;
};

#endif // DUMMYSOURCESBACKEND_H
