/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "BackendNotifierModule.h"

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

BackendNotifierModule::BackendNotifierModule(QObject *parent)
    : QObject(parent)
{
}

BackendNotifierModule::~BackendNotifierModule() = default;

QDateTime BackendNotifierModule::lastUpdate() const
{
    return KSharedConfig::openStateConfig(u"discovernotifierstaterc"_s)->group(u"General"_s).readEntry(u"lastUpdate"_s, QDateTime());
}

#include "moc_BackendNotifierModule.cpp"
