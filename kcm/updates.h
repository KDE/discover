/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include <KSharedConfig>

class UpdatesData;
class UpdatesSettings;

class Updates : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(UpdatesSettings *updatesSettings READ updatesSettings CONSTANT)

public:
    explicit Updates(QObject *parent = nullptr, const QVariantList &list = QVariantList());
    ~Updates() override;

    UpdatesSettings *updatesSettings() const;

private:
    UpdatesData *m_data;
};
