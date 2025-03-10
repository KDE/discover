/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QtTypes>
#include <Transaction/Transaction.h>
#include <sysupdate1.h>

constexpr auto SYSUPDATE1_SERVICE = QLatin1String("org.freedesktop.sysupdate1");
using SystemdSysupdateUpdateReply = QDBusPendingReply<QString, qulonglong, QDBusObjectPath>;

class SystemdSysupdateTransaction : public Transaction
{
public:
    SystemdSysupdateTransaction(AbstractResource *resource, SystemdSysupdateUpdateReply &reply);

    void cancel() override;

private:
    org::freedesktop::sysupdate1::Job *m_job;
};
