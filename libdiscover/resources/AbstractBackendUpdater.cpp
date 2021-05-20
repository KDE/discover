/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractBackendUpdater.h"
#include "AbstractResource.h"

AbstractBackendUpdater::AbstractBackendUpdater(QObject *parent)
    : QObject(parent)
{
}

void AbstractBackendUpdater::cancel()
{
    Q_ASSERT(isCancelable() && "only call cancel when cancelable");
    Q_ASSERT(false && "if it can be canceled, then ::cancel() must be implemented");
}

void AbstractBackendUpdater::fetchChangelog() const
{
    const auto toUpd = toUpdate();
    for (auto res : toUpd) {
        res->fetchChangelog();
    }
}

void AbstractBackendUpdater::enableNeedsReboot()
{
    if (m_needsReboot)
        return;

    m_needsReboot = true;
    Q_EMIT needsRebootChanged();
}

bool AbstractBackendUpdater::needsReboot() const
{
    return m_needsReboot;
}

void AbstractBackendUpdater::setOfflineUpdates(bool useOfflineUpdates)
{
    Q_UNUSED(useOfflineUpdates);
}
