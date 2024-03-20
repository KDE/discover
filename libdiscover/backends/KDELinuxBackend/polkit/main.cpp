// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <sys/stat.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>

#include <polkitqt1-agent-session.h>
#include <polkitqt1-authority.h>

#include "deletelater.h"

using namespace Qt::StringLiterals;

constexpr auto SYSTEMD_SYSUPDATE = "/usr/lib/systemd/systemd-sysupdate"_L1;

void ensureEspIsMounted()
{
    // systemd unmounts the ESP if unused for a while. Make sure it is mounted or sysupdate errors out during early stages.
    struct stat st {
    };
    stat("/efi/EFI", &st);
}

class Helper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.discover.kde.linux")

private:
    [[nodiscard]] auto makeProcess()
    {
        ensureEspIsMounted();

        auto sysupdate = makeDeleteLaterShared<QProcess>(this);
        sysupdate->setProgram(SYSTEMD_SYSUPDATE);
        auto environment = sysupdate->processEnvironment();
        environment.insert(u"TAR_OPTIONS"_s, u"--zstd"_s);
        sysupdate->setProcessEnvironment(environment);
        return sysupdate;
    }

    [[nodiscard]] std::tuple<QDBusConnection, QDBusMessage> delayReply()
    {
        setDelayedReply(true);
        return {connection(), message()};
    }

public Q_SLOTS:
    void update()
    {
        auto loopLock = std::make_shared<QEventLoopLocker>();

        if (!isAuthorized(u"update"_s)) {
            qWarning() << "not authorized";
            sendErrorReply(QDBusError::AccessDenied);
            return;
        }

        auto sysupdate = makeProcess();
        sysupdate->setProcessChannelMode(QProcess::ForwardedChannels);
        sysupdate->setArguments({u"--json=pretty"_s, u"--verify=no"_s, u"update"_s});

        auto [connection, msg] = delayReply();

        connect(sysupdate.get(), &QProcess::finished, this, [loopLock, connection, msg, sysupdate](int exitCode, QProcess::ExitStatus exitStatus) {
            qWarning() << "update" << exitCode << exitStatus << sysupdate->readAllStandardOutput();
            if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                connection.send(msg.createErrorReply(QDBusError::Failed, QStringLiteral("Failed to update")));
                return;
            }

            connection.send(msg.createReply());
        });

        sysupdate->start();
    }

    void check_new()
    {
        auto loopLock = std::make_shared<QEventLoopLocker>();

        if (!isAuthorized(u"check_new"_s)) {
            qWarning() << "not authorized";
            sendErrorReply(QDBusError::AccessDenied);
            return;
        }

        auto sysupdate = makeProcess();
        sysupdate->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        sysupdate->setArguments({u"--json=pretty"_s, u"--verify=no"_s, u"check-new"_s});

        auto [connection, msg] = delayReply();

        connect(sysupdate.get(), &QProcess::finished, this, [loopLock, connection, msg, sysupdate](int exitCode, QProcess::ExitStatus exitStatus) {
            auto data = QString::fromUtf8(sysupdate->readAllStandardOutput().trimmed());
            qWarning() << "check-new" << exitCode << exitStatus << data;
            if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                connection.send(msg.createErrorReply(QDBusError::Failed, QStringLiteral("Failed to check for updates")));
                return;
            }

            auto reply = msg.createReply() << data;
            connection.send(reply);
        });

        sysupdate->start();
    }

private:
    [[nodiscard]] bool isAuthorized(const QString &action)
    {
        auto authority = PolkitQt1::Authority::instance();
        auto result = authority->checkAuthorizationSyncWithDetails("org.kde.discover.kde.linux."_L1 + action,
                                                                   PolkitQt1::SystemBusNameSubject(message().service()),
                                                                   PolkitQt1::Authority::AllowUserInteraction,
                                                                   {});

        if (authority->hasError()) {
            qWarning() << authority->lastError() << authority->errorDetails();
            authority->clearError();
            return false;
        }

        switch (result) {
        case PolkitQt1::Authority::Yes:
            return true;
        case PolkitQt1::Authority::Unknown:
        case PolkitQt1::Authority::No:
        case PolkitQt1::Authority::Challenge:
            break;
        }
        return false;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(true);

    Helper helper;

    if (!QDBusConnection::systemBus().registerObject(QStringLiteral("/"), &helper, QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register the daemon object" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }
    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.kde.discover.kde.linux"))) {
        qWarning() << "Failed to register the service" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }

    return app.exec();
}

#include "main.moc"
