/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef STEAMOSTRANSACTION_H
#define STEAMOSTRANSACTION_H

#include <QPointer>
#include <Transaction/Transaction.h>

#include "atomupd1.h"

class QTimer;
class SteamOSResource;
class SteamOSTransaction : public Transaction
{
    Q_OBJECT
public:
    SteamOSTransaction(SteamOSResource *app, Role role, ComSteampoweredAtomupd1Interface *interface);

    void cancel() override;

Q_SIGNALS:
    void needReboot();

private Q_SLOTS:
    void pollStatus();

private:
    void finishTransaction();

    SteamOSResource *m_app;
    QPointer<ComSteampoweredAtomupd1Interface> m_interface; // Interface to atomupd dbus api
    QTimer *m_timer; // Timer to poll for progress, state, etc.
};

#endif // STEAMOSTRANSACTION_H
