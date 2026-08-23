/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "helper.h"

#include "sysupdateerror.h"
#include "transferconfig.h"

#include <KAuth/HelperSupport>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

using namespace KAuth;
using namespace Qt::StringLiterals;

namespace
{
ActionReply failure(SysupdateError::Code code, const QString &detail = {})
{
    ActionReply reply = ActionReply::HelperErrorReply();
    reply.setErrorCode(static_cast<ActionReply::Error>(code));
    reply.setErrorDescription(detail);
    return reply;
}

bool writeDropIn(const QString &path, const QStringList &versions)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }

    QByteArray contents = "# Managed by the Discover Update module.\n[Transfer]\n";
    for (const QString &version : versions) {
        contents += "ProtectVersion=" + version.toUtf8() + '\n';
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit()) {
        return false;
    }

    return QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
}

void removeDropIn(const QString &path)
{
    QFile::remove(path);
    QDir().rmdir(QFileInfo(path).absolutePath());
}
}

ActionReply SysupdateHelper::save(const QVariantMap &arguments)
{
    QStringList versions = arguments.value(u"versions"_s).toStringList();
    versions.removeDuplicates();
    versions.sort();

    for (const QString &version : std::as_const(versions)) {
        if (!TransferConfig::isValidVersion(version)) {
            return failure(SysupdateError::InvalidVersion, version);
        }
    }

    const TransferConfig config = TransferConfig::scan();
    if (config.isEmpty()) {
        return failure(SysupdateError::NoTransferDefinitions);
    }

    const QStringList orphans = config.orphanedDropIns();
    for (const QString &orphan : orphans) {
        removeDropIn(orphan);
    }

    // Failing halfway leaves some transfers protecting a version and others not; the module
    // detects that and offers to apply again.
    const QList<TransferConfig::Transfer> transfers = config.transfers();
    for (const TransferConfig::Transfer &transfer : transfers) {
        if (versions.isEmpty()) {
            removeDropIn(transfer.dropIn);
        } else if (!writeDropIn(transfer.dropIn, versions)) {
            return failure(SysupdateError::WriteFailed, transfer.dropIn);
        }
    }

    return ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.discover.sysupdate", SysupdateHelper)
