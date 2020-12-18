/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OSTreeRPMBackend.h"

DISCOVER_BACKEND_PLUGIN(OSTreeRPMBackend)

OSTreeRPMBackend::OSTreeRPMBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
{
}

#include "OSTreeRPMBackend.moc"
