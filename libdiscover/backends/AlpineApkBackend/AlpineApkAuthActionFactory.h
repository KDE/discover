/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef AlpineApkAuthActionFactory_H
#define AlpineApkAuthActionFactory_H

#include <QString>
#include <QVariant>

#include <KAuth/Action>
#include <KAuth/ActionReply>
#include <KAuth/ExecuteJob>

namespace ActionFactory {

KAuth::ExecuteJob *createUpdateAction(const QString &fakeRoot);
KAuth::ExecuteJob *createUpgradeAction(bool onlySimulate = false);
KAuth::ExecuteJob *createAddAction(const QString &pkgName);
KAuth::ExecuteJob *createDelAction(const QString &pkgName);
KAuth::ExecuteJob *createRepoconfigAction(const QVariant &repoUrls);

} // namespace ActionFactory

#endif
