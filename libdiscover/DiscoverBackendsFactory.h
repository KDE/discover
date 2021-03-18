/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DISCOVERBACKENDSFACTORY_H
#define DISCOVERBACKENDSFACTORY_H

#include "discovercommon_export.h"
#include <QList>

class QCommandLineParser;
class QStringList;
class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT DiscoverBackendsFactory
{
public:
    DiscoverBackendsFactory();

    QVector<AbstractResourcesBackend *> backend(const QString &name) const;
    QVector<AbstractResourcesBackend *> allBackends() const;
    QStringList allBackendNames(bool whitelist = true, bool allowDummy = false) const;
    int backendsCount() const;

    static void setupCommandLine(QCommandLineParser *parser);
    static void processCommandLine(QCommandLineParser *parser, bool test);
    static void setRequestedBackends(const QStringList &backends);
    static bool hasRequestedBackends();

private:
    QVector<AbstractResourcesBackend *> backendForFile(const QString &path, const QString &name) const;
};

#endif // MUONBACKENDSFACTORY_H
