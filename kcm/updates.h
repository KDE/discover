/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include <KSharedConfig>
#include <discoversettings.h>
#include <updatessettings.h>
class UpdatesData;
class DiscoverData;

class Updates : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(UpdatesSettings *updatesSettings READ updatesSettings CONSTANT)
    Q_PROPERTY(DiscoverSettings *discoverSettings READ discoverSettings CONSTANT)

public:
    explicit Updates(QObject *parent = nullptr, const QVariantList &list = QVariantList());
    ~Updates() override;

    UpdatesSettings *updatesSettings() const;
    DiscoverSettings *discoverSettings() const;

private:
    UpdatesData *const m_data;
    DiscoverData *const m_discoverData;
};
