/*
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "DummyNotifier.h"

#include <QDebug>

DummyNotifier::DummyNotifier(QObject *parent)
    : BackendNotifierModule(parent)
{
}

DummyNotifier::~DummyNotifier()
{
}

void DummyNotifier::recheckSystemUpdateNeeded()
{
    Q_EMIT foundUpdates();
}
