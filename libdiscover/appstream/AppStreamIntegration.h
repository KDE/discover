/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"

#include <AppStreamQt/pool.h>
#include <AppStreamQt/release.h>
#include <KOSRelease>
#include <QObject>

#include <optional>

class DISCOVERCOMMON_EXPORT AppStreamIntegration : public QObject
{
    Q_OBJECT
public:
    static AppStreamIntegration *global();

    std::optional<AppStream::Release> getDistroUpgrade(AppStream::Pool *pool, std::optional<QString> IdOverride = {});
    KOSRelease *osRelease()
    {
        return &m_osrelease;
    }

private:
    KOSRelease m_osrelease;

    AppStreamIntegration()
    {
    }
};
