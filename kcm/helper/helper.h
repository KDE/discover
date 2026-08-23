/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KAuth/ActionReply>

#include <QObject>
#include <QVariantMap>

class SysupdateHelper : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    KAuth::ActionReply save(const QVariantMap &arguments);
};
