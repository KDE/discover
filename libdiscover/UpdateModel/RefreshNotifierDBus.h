// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QString>

namespace RefreshNotifierDBus
{
constexpr auto path = QLatin1String("/");
constexpr auto interface = QLatin1String("org.kde.discover");
constexpr auto notifyNotifier = QLatin1String("NotifyNotifier");
constexpr auto notifyApp = QLatin1String("NotifyApp");

inline void notify(const QString &target)
{
    auto signal = QDBusMessage::createSignal(RefreshNotifierDBus::path, RefreshNotifierDBus::interface, target);
    QDBusConnection::sessionBus().send(signal);
}
} // namespace RefreshNotifierDBus
