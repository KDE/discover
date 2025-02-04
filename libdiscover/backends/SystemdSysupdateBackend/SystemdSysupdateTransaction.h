/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QtTypes>
#include <Transaction/Transaction.h>
#include <sysupdate1.h>

const auto SYSUPDATE1_SERVICE = QStringLiteral("org.freedesktop.sysupdate1");
typedef QDBusPendingReply<QString, qulonglong, QDBusObjectPath> SystemdSysupdateUpdateReply;

class SystemdSysupdateTransaction : public Transaction
{
public:
    SystemdSysupdateTransaction(AbstractResource *resource, SystemdSysupdateUpdateReply &reply);

    void cancel() override;
    ~SystemdSysupdateTransaction() override;

private:
    org::freedesktop::sysupdate1::Job *m_job;
};
