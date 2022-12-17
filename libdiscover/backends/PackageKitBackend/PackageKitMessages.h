/*
 *   SPDX-FileCopyrightText: 2012-2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <PackageKit/Transaction>

namespace PackageKitMessages
{
QString errorMessage(PackageKit::Transaction::Error error, const QString &errorMessage);
QString restartMessage(PackageKit::Transaction::Restart restart, const QString &p);
QString restartMessage(PackageKit::Transaction::Restart restart);
QString statusMessage(PackageKit::Transaction::Status status);
QString statusDetail(PackageKit::Transaction::Status status);
QString updateStateMessage(PackageKit::Transaction::UpdateState state);
QString info(PackageKit::Transaction::Info info);
}
