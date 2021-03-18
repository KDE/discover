/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BACKENDNOTIFIERFACTORY_H
#define BACKENDNOTIFIERFACTORY_H

#include <QList>
#include <QString>

class BackendNotifierModule;

class BackendNotifierFactory
{
public:
    BackendNotifierFactory();

    QList<BackendNotifierModule *> allBackends() const;
};

#endif // BACKENDNOTIFIERFACTORY_H
