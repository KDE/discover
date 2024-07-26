// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include "discovercommon_export.h"

#include <QObject>
#include <QTimer>

// Sends a broadcast over dbus to refresh the DiscoverNotifier when our updatable count changes.
class DISCOVERCOMMON_EXPORT RefreshNotifier : public QObject
{
    Q_OBJECT
public:
    RefreshNotifier(QObject *parent = nullptr);
    ~RefreshNotifier() override;
    Q_DISABLE_COPY_MOVE(RefreshNotifier)

private Q_SLOTS:
    void onNotify();

private:
    QTimer m_timer;
};
