/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QSet>
#include <QTimer>
#include <QUrl>

struct StreamResult;

class DiscoverExporter : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverExporter();
    ~DiscoverExporter() override;

    void setExportPath(const QUrl &url);

public Q_SLOTS:
    void fetchResources();
    void exportResources(const QVector<StreamResult> &resources);

Q_SIGNALS:
    void exportDone();

private:
    QUrl m_path;
    const QSet<QByteArray> m_excludedProperties;
};
