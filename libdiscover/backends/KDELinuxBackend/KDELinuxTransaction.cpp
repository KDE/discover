// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "KDELinuxTransaction.h"

#include <chrono>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>

#include "debug.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

KDELinuxTransaction::KDELinuxTransaction(QObject *parent, const std::shared_ptr<AbstractResource> &resource)
    : Transaction(parent, resource.get(), Transaction::InstallRole, {})
    , m_resource(resource)
{
    setCancellable(false);
    setStatus(Status::SetupStatus);

    setStatus(Status::DownloadingStatus);
    auto msg = QDBusMessage::createMethodCall(u"org.kde.discover.kde.linux"_s, u"/"_s, u"org.kde.discover.kde.linux"_s, u"update"_s);
    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg, std::numeric_limits<int>::max()));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusReply<void> reply = *watcher;
        if (!reply.isValid() || reply.error().isValid()) {
            qCWarning(KDELINUX) << "Failed to install" << reply.error().message();
            Q_EMIT passiveMessage(reply.error().message());
            setStatus(Status::DoneWithErrorStatus);
            return;
        }
        setStatus(Status::DoneStatus);
    });
}

void KDELinuxTransaction::cancel()
{
}
