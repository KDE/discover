/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef MUONEXPORTER_H
#define MUONEXPORTER_H

#include <QUrl>
#include <QSet>
#include <QTimer>

class AbstractResource;

class MuonExporter : public QObject
{
    Q_OBJECT
    public:
        explicit MuonExporter();
        ~MuonExporter() override;

        void setExportPath(const QUrl& url);

    public Q_SLOTS:
        void fetchResources();
        void exportResources(const QVector<AbstractResource*>& resources);

    Q_SIGNALS:
        void exportDone();

    private:
        QUrl m_path;
        const QSet<QByteArray> m_exculdedProperties;
};

#endif // MUONEXPORTER_H
