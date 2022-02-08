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
public:
    UnattendedUpdates(DiscoverNotifier *parent);
    ~UnattendedUpdates() override;

private:
    void checkNewState();
    void triggerUpdate(int timeoutId);

    std::optional<int> m_idleTimeoutId;
};
