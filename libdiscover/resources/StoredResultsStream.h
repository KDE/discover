/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "ResourcesModel.h"

class DISCOVERCOMMON_EXPORT StoredResultsStream : public AggregatedResultsStream
{
    Q_OBJECT
public:
    StoredResultsStream(const QSet<ResultsStream *> &streams);

    QVector<AbstractResource *> resources() const;

Q_SIGNALS:
    void finishedResources(const QVector<AbstractResource *> &resources);

private:
    QVector<AbstractResource *> m_resources;
};
