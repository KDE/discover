/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include "resources/AbstractResource.h"
#include <QModelIndex>
#include <QUrl>

class AbstractResource;

class DISCOVERCOMMON_EXPORT ScreenshotsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource *application READ resource WRITE setResource NOTIFY resourceChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        ThumbnailUrl = Qt::UserRole + 1,
        ScreenshotUrl,
        IsAnimatedRole,
    };

    explicit ScreenshotsModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;

    AbstractResource *resource() const;
    void setResource(AbstractResource *res);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_SCRIPTABLE QUrl screenshotAt(int row) const;
    int count() const;

    Q_INVOKABLE void remove(const QUrl &url);

private Q_SLOTS:
    void screenshotsFetched(const Screenshots &screenshots);

Q_SIGNALS:
    void countChanged();
    void resourceChanged(const AbstractResource *resource);

private:
    AbstractResource *m_resource;
    Screenshots m_screenshots;
};
