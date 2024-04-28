/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamIntegration.h"

#include <AppStreamQt/systeminfo.h>
#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>

using namespace Qt::StringLiterals;

AppStreamIntegration *AppStreamIntegration::global()
{
    static AppStreamIntegration *var = nullptr;
    if (!var) {
        var = new AppStreamIntegration;
    }

    return var;
}

std::optional<AppStream::Release> AppStreamIntegration::getDistroUpgrade(AppStream::Pool *pool, std::optional<QString> IdOverride)
{
    QString distroId;
    if (IdOverride) {
        distroId = *IdOverride;
    } else {
        distroId = AppStream::SystemInfo::currentDistroComponentId();
    }

    // Look at releases to see if we have a new major version available.
    const auto distroComponents = pool->componentsById(distroId);
    if (distroComponents.isEmpty()) {
        qWarning() << "AppStreamIntegration: No distro component found for" << distroId;
        return std::nullopt;
    }

    KConfigGroup settings(KSharedConfig::openConfig(QStringLiteral("discoverrc")), u"DistroUpgrade"_s);
    bool allowPreRelease = settings.readEntry<bool>("AllowPreRelease", false);

    QString currentVersion = osRelease()->versionId();
    std::optional<AppStream::Release> nextRelease;
    for (const AppStream::Component &dc : distroComponents) {
        const auto releases = dc.releasesPlain().entries();
        for (const auto &r : releases) {
            // Only look at stable releases unless requested
            if (!(r.kind() == AppStream::Release::KindStable || (r.kind() == AppStream::Release::KindDevelopment && allowPreRelease))) {
                continue;
            }

            // Let's look at this potentially new version
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

#include "moc_AppStreamIntegration.cpp"
