/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef OSTREERPM1BACKEND_H
#define OSTREERPM1BACKEND_H

#include <resources/AbstractResourcesBackend.h>

#include "rpmostree1.h"

class OSTreeRPMBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    OSTreeRPMBackend(QObject* parent = nullptr);

private:
    OrgProjectatomicRpmostree1OSInterface m_interface;
};

#endif
