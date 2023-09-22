/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamIntegration.h"

#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>

AppStreamIntegration *AppStreamIntegration::global()
{
    static AppStreamIntegration *var = nullptr;
    if (!var) {
        var = new AppStreamIntegration;
    }

    return var;
}

std::optional<AppStream::Release> AppStreamIntegration::getDistroUpgrade(AppStream::Pool *pool)
{
    QString distroId = AppStream::Utils::currentDistroComponentId();

    // Look at releases to see if we have a new major version available.
    const auto distroComponents = pool->componentsById(distroId);
    if (distroComponents.isEmpty()) {
        qWarning() << "AppStreamIntegration: No distro component found for" << distroId;
        return std::nullopt;
    }

    KConfigGroup settings(KSharedConfig::openConfig(QStringLiteral("discoverrc")), "DistroUpgrade");
    bool allowPreRelease = settings.readEntry<bool>("AllowPreRelease", false);

    QString currentVersion = osRelease()->versionId();
    std::optional<AppStream::Release> nextRelease;
    for (const auto list = distroComponents.toList(); const AppStream::Component &dc : list) {
#if ASQ_CHECK_VERSION(1, 0, 0)
        const auto releases = dc.releasesPlain().entries();
#else
        const auto releases = dc.releases();
#endif
        for (const auto &r : releases) {
            // Only look at stable releases unless requested
            if (!allowPreRelease && r.kind() != AppStream::Release::KindStable) {
                continue;
            }

            // Let's look at this potentially new verson
            const QString newVersion = r.version();
            if (AppStream::Utils::vercmpSimple(newVersion, currentVersion) > 0) {
                if (!nextRelease) {
                    // No other newer version found yet so let's pick this one
                    nextRelease = r;
                    qInfo() << "Found new major release:" << newVersion;
                } else if (AppStream::Utils::vercmpSimple(nextRelease->version(), newVersion) > 0) {
                    // We only offer updating to the very next major release so
                    // we pick the smallest of all the newest versions
                    nextRelease = r;
                    qInfo() << "Found a closer new major release:" << newVersion;
                }
            }
        }
    }

    return nextRelease;
}
