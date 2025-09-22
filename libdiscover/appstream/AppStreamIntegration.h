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
#include <QFuture>
#include <QObject>
#include <QSet>

#include <optional>

class DISCOVERCOMMON_EXPORT AppStreamIntegration : public QObject
{
    Q_OBJECT
public:
    enum class IconState {
        Present,
        NotPresent,
        Pending
    };
    Q_ENUM(IconState)

    static AppStreamIntegration *global();

    std::optional<AppStream::Release> getDistroUpgrade(AppStream::Pool *pool, std::optional<QString> IdOverride = {});
    KOSRelease *osRelease()
    {
        return &m_osrelease;
    }

    IconState kIconLoaderHasIcon(const QString &name);
    bool hasIcons() const
    {
        return m_icons.isFinished();
    }

Q_SIGNALS:
    void iconsChanged();

private:
    KOSRelease m_osrelease;
    QFuture<QSet<QString>> m_icons;
    bool m_iconFetchStarted = false;

    AppStreamIntegration()
    {
    }
};
