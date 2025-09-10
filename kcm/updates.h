/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KQuickManagedConfigModule>

#include <KSharedConfig>
#include <discoversettings.h>
#include <updatessettings.h>

class UpdatesData;
class DiscoverData;

class Updates : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(UpdatesSettings *updatesSettings READ updatesSettings CONSTANT)
    Q_PROPERTY(DiscoverSettings *discoverSettings READ discoverSettings CONSTANT)
    Q_PROPERTY(bool mandatoryRebootAfterUpdate READ mandatoryRebootAfterUpdate CONSTANT)

public:
    explicit Updates(QObject *parent, const KPluginMetaData &data);

    UpdatesSettings *updatesSettings() const;
    DiscoverSettings *discoverSettings() const;

    /* Returns true if we're running on a system that needs rebooting after updating.
     * It is used to hide the setting asking the user when to apply the updates
     * because it's mandatory at times.*/
    bool mandatoryRebootAfterUpdate() const;

private:
    UpdatesData *const m_data;
    DiscoverData *const m_discoverData;
};
