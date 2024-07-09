/*
 *   SPDX-FileCopyrightText: 2020 Alexey Min <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KLocalizedString>
#include <kauth_version.h>

#include "AlpineApkAuthActionFactory.h"
#include "alpineapk_backend_logging.h"

namespace ActionFactory {

static KAuth::Action createAlpineApkKAuthAction()
{
    KAuth::Action action(QStringLiteral("org.kde.discover.alpineapkbackend.pkgmgmt"));
    action.setHelperId(QStringLiteral("org.kde.discover.alpineapkbackend"));
    if (!action.isValid()) {
        qCWarning(LOG_ALPINEAPK) << "Created KAuth action is not valid!";
        return action;
    }

    // set action description
    static const KAuth::Action::DetailsMap details{
        { KAuth::Action::AuthDetail::DetailMessage, i18n("Package management") }
    };
    action.setDetailsV2(details);

    // change default timeout to 1 minute, bcause default DBus timeout
    //   of 25 seconds is not enough
    action.setTimeout(1 * 60 * 1000);

    return action;
}

KAuth::ExecuteJob *createUpdateAction(const QString &fakeRoot)
{
    KAuth::Action action = createAlpineApkKAuthAction();
    if (!action.isValid()) {
        return nullptr;
    }
    // update-action specific details
    action.setTimeout(2 * 60 * 1000); // 2 minutes
    action.addArgument(QLatin1String("pkgAction"), QLatin1String("update"));
    action.addArgument(QLatin1String("fakeRoot"), fakeRoot);
    return action.execute();
}

KAuth::ExecuteJob *createUpgradeAction(bool onlySimulate)
{
    KAuth::Action action = createAlpineApkKAuthAction();
    if (!action.isValid()) {
        return nullptr;
    }
    action.setTimeout(3 * 60 * 60 * 1000); // 3 hours, system upgrade can take really long
    action.addArgument(QLatin1String("pkgAction"), QLatin1String("upgrade"));
    action.addArgument(QLatin1String("onlySimulate"), onlySimulate);
    return action.execute();
}

KAuth::ExecuteJob *createAddAction(const QString &pkgName)
{
    KAuth::Action action = createAlpineApkKAuthAction();
    if (!action.isValid()) {
        return nullptr;
    }
    action.setTimeout(1 * 60 * 60 * 1000); // 1 hour, in case package is really big?
    action.addArgument(QLatin1String("pkgAction"), QLatin1String("add"));
    action.addArgument(QLatin1String("pkgName"), pkgName);
    return action.execute();
}

KAuth::ExecuteJob *createDelAction(const QString &pkgName)
{
    KAuth::Action action = createAlpineApkKAuthAction();
    if (!action.isValid()) {
        return nullptr;
    }
    action.setTimeout(1 * 60 * 60 * 1000); // although deletion is almost instant
    action.addArgument(QLatin1String("pkgAction"), QLatin1String("del"));
    action.addArgument(QLatin1String("pkgName"), pkgName);
    return action.execute();
}

KAuth::ExecuteJob *createRepoconfigAction(const QVariant &repoUrls)
{
    KAuth::Action action = createAlpineApkKAuthAction();
    if (!action.isValid()) {
        return nullptr;
    }
    // should be instant, writes few lines to /etc/apk/repositories
    action.setTimeout(1 * 60 * 1000); // 1 minute
    action.addArgument(QLatin1String("pkgAction"), QLatin1String("repoconfig"));
    action.addArgument(QLatin1String("repoList"), repoUrls);
    return action.execute();
}

} // namespace ActionFactory
