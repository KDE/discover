/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <optional>

#include <QObject>

class DiscoverNotifier;

class UnattendedUpdates : public QObject
{
    Q_OBJECT

    // Wraps a KIdleTime id to ensure it is released whenever necessary.
    class IdleHandle
    {
    public:
        IdleHandle(const std::chrono::milliseconds &idleTimeout);
        ~IdleHandle();
        Q_DISABLE_COPY_MOVE(IdleHandle)
        int m_id;
    };

public:
    UnattendedUpdates(DiscoverNotifier *parent);

private:
    void checkNewState();
    void triggerUpdate(int timeoutId);

    std::optional<IdleHandle> m_idleTimeoutId;
    DiscoverNotifier *const m_notifier;
};
