/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
 *   Copyright © 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "PackageKitNotifier.h"

#include <QTimer>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QProcess>
#include <QTextStream>
#include <QDebug>
#include <KNotification>
#include <PackageKit/Daemon>
#include <PackageKit/Offline>
#include <QDBusInterface>
#include <QFile>
#include <QFileSystemWatcher>
#include <KLocalizedString>
#include <KDesktopFile>
#include <KConfigGroup>

#include "pk-offline-private.h"

PackageKitNotifier::PackageKitNotifier(QObject* parent)
    : BackendNotifierModule(parent)
    , m_securityUpdates(0)
    , m_normalUpdates(0)
{
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::transactionListChanged, this, &PackageKitNotifier::transactionListChanged);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::restartScheduled, this, &PackageKitNotifier::nowNeedsReboot);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::changed, this, [this]{
        if (PackageKit::Daemon::global()->offline()->updateTriggered())
            nowNeedsReboot();
    });

    //Check if there's packages after 5'
    QTimer::singleShot(5 * 60 * 1000, this, &PackageKitNotifier::refreshDatabase);

    QTimer *regularCheck = new QTimer(this);
    connect(regularCheck, &QTimer::timeout, this, &PackageKitNotifier::refreshDatabase);

    const QString aptconfig = QStandardPaths::findExecutable(QStringLiteral("apt-config"));
    if (!aptconfig.isEmpty()) {
        checkAptVariable(aptconfig, QLatin1String("Apt::Periodic::Update-Package-Lists"), [regularCheck](const QStringRef& value) {
            bool ok;
            const int days = value.toInt(&ok);
            if (!ok || days == 0) {
                regularCheck->setInterval(24 * 60 * 60 * 1000); //refresh at least once every day
                regularCheck->start();
                qWarning() << "couldn't understand value for timer:" << value;
            }

            //if the setting is not empty, refresh will be carried out by unattended-upgrade
            //https://wiki.debian.org/UnattendedUpgrades
        });
    } else {
        regularCheck->setInterval(24 * 60 * 60 * 1000); //refresh at least once every day
        regularCheck->start();
    }

    QTimer::singleShot(3000, this, &PackageKitNotifier::checkOfflineUpdates);

    m_recheckTimer = new QTimer(this);
    m_recheckTimer->setInterval(200);
    m_recheckTimer->setSingleShot(true);
    connect(m_recheckTimer, &QTimer::timeout, this, &PackageKitNotifier::recheckSystemUpdate);

    QFileSystemWatcher* watcher = new QFileSystemWatcher(this);
    watcher->addPath(QStringLiteral(PK_OFFLINE_ACTION_FILENAME));
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &PackageKitNotifier::nowNeedsReboot);

    QTimer::singleShot(100, this, [this](){
    if (QFile::exists(QStringLiteral(PK_OFFLINE_ACTION_FILENAME)))
        nowNeedsReboot();
    });
}

PackageKitNotifier::~PackageKitNotifier()
{
}

void PackageKitNotifier::checkOfflineUpdates()
{
    if (!QFile::exists(QStringLiteral(PK_OFFLINE_RESULTS_FILENAME))) {
        return;
    }
    qDebug() << "found offline update results at " << PK_OFFLINE_RESULTS_FILENAME;

    KDesktopFile file(QStringLiteral(PK_OFFLINE_RESULTS_FILENAME));
    KConfigGroup group(&file, PK_OFFLINE_RESULTS_GROUP);

    const bool success = group.readEntry("Success", false);
    const QString packagesJoined = group.readEntry("Packages");
    const auto packages = packagesJoined.splitRef(QLatin1Char(','));
    if (!success) {
        const QString errorDetails = group.readEntry("ErrorDetails");

        KNotification *notification = new KNotification(QStringLiteral("offlineupdate-failed"), KNotification::Persistent | KNotification::DefaultEvent);
        notification->setIconName(QStringLiteral("error"));
        notification->setText(i18n("Offline Updates"));
        notification->setText(i18np("Failed to update %1 package\n%2", "Failed to update %1 packages\n%2",
                                    packages.count(), errorDetails));
        notification->setActions(QStringList{QLatin1String("Open Discover")});

        connect(notification, &KNotification::action1Activated, this, [] () {
            QProcess::startDetached(QStringLiteral("plasma-discover"));
        });
        connect(notification, &KNotification::action2Activated, this, [this] () {
            auto trans = PackageKit::Daemon::global()->repairSystem();
            connect(trans, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error /*error*/, const QString &details){
                KNotification::event(QStringLiteral("offlineupdate-repair-failed"), i18n("Repair Failed"), i18n("Please report to your distribution: %1", details), {}, KNotification::Persistent, QStringLiteral("org.kde.discovernotifier"));
            });
        });

        notification->sendEvent();
    } else {
        KNotification *notification = new KNotification(QStringLiteral("offlineupdate-successful"));
        notification->setIconName(QStringLiteral("system-software-update"));
        notification->setTitle(i18n("Offline Updates"));
        notification->setText(i18np("Successfully updated %1 package", "Successfully updated %1 packages", packages.count()));
        notification->setActions(QStringList{QLatin1String("Open Discover")});

        connect(notification, &KNotification::action1Activated, this, [] () {
            QProcess::startDetached(QStringLiteral("plasma-discover"));
        });

        notification->sendEvent();
    }
}

void PackageKitNotifier::recheckSystemUpdateNeeded()
{
    static bool first = true;
    if (first) {
        //PKQt will emit these signals when it starts (bug?) and would trigger the system recheck before we've ever checked at all
        connect(PackageKit::Daemon::global(), &PackageKit::Daemon::networkStateChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
        connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
        first = false;
    }

    if (PackageKit::Daemon::global()->offline()->updateTriggered())
        return;

    m_recheckTimer->start();
}

void PackageKitNotifier::recheckSystemUpdate()
{
    if (PackageKit::Daemon::global()->isRunning()) {
        PackageKit::Daemon::getUpdates();
    }
}

void PackageKitNotifier::setupGetUpdatesTransaction(PackageKit::Transaction* trans)
{
    qDebug() << "using..." << trans << trans->tid().path();

    trans->setProperty("normalUpdates", 0);
    trans->setProperty("securityUpdates", 0);
    connect(trans, &PackageKit::Transaction::package, this, &PackageKitNotifier::package);
    connect(trans, &PackageKit::Transaction::finished, this, &PackageKitNotifier::finished);
}

void PackageKitNotifier::package(PackageKit::Transaction::Info info, const QString &/*packageID*/, const QString &/*summary*/)
{
    PackageKit::Transaction * trans = qobject_cast<PackageKit::Transaction *>(sender());

    switch (info) {
        case PackageKit::Transaction::InfoBlocked:
            break; //skip, we ignore blocked updates
        case PackageKit::Transaction::InfoSecurity:
            trans->setProperty("securityUpdates", trans->property("securityUpdates").toInt()+1);
            break;
        default:
            trans->setProperty("normalUpdates", trans->property("normalUpdates").toInt()+1);
            break;
    }
}

void PackageKitNotifier::finished(PackageKit::Transaction::Exit /*exit*/, uint)
{
    const PackageKit::Transaction * trans = qobject_cast<PackageKit::Transaction *>(sender());

    const uint normalUpdates = trans->property("normalUpdates").toInt();
    const uint securityUpdates = trans->property("securityUpdates").toInt();
    const bool changed = normalUpdates != m_normalUpdates || securityUpdates != m_securityUpdates;

    m_normalUpdates = normalUpdates;
    m_securityUpdates = securityUpdates;

    if (changed) {
        Q_EMIT foundUpdates();
    }
}

bool PackageKitNotifier::hasUpdates()
{
    return m_normalUpdates > 0;
}

bool PackageKitNotifier::hasSecurityUpdates()
{
    return m_securityUpdates > 0;
}


void PackageKitNotifier::onDistroUpgrade(PackageKit::Transaction::DistroUpgrade /*type*/, const QString& name, const QString& description)
{
    auto a = new UpgradeAction(name, description, this);
    connect(a, &UpgradeAction::triggered, this, [] (const QString &name) {
        PackageKit::Daemon::upgradeSystem(name, PackageKit::Transaction::UpgradeKindDefault);
    });
    Q_EMIT foundUpgradeAction(a);
}

void PackageKitNotifier::refreshDatabase()
{
    if (!m_refresher) {
        m_refresher = PackageKit::Daemon::refreshCache(false);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    }

    if (!m_distUpgrades && (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleUpgradeSystem)) {
        m_distUpgrades = PackageKit::Daemon::getDistroUpgrades();
        connect(m_distUpgrades, &PackageKit::Transaction::distroUpgrade, this, &PackageKitNotifier::onDistroUpgrade);
    }
}

QProcess* PackageKitNotifier::checkAptVariable(const QString &aptconfig, const QLatin1String& varname, const std::function<void(const QStringRef& val)> &func)
{
    QProcess* process = new QProcess;
    process->start(aptconfig, {QStringLiteral("dump")});
    connect(process, static_cast<void(QProcess::*)(int)>(&QProcess::finished), this, [func, process, varname](int code) {
        if (code != 0)
            return;

        QRegularExpression rx(QLatin1Char('^') + varname + QStringLiteral(" \"(.*?)\"$"));
        QTextStream stream(process);
        QString line;
        while (stream.readLineInto(&line)) {
            const auto match = rx.match(line);
            if (match.hasMatch()) {
                func(match.capturedRef(1));
                return;
            }
        }
        func({});
    });
    connect(process, static_cast<void(QProcess::*)(int)>(&QProcess::finished), process, &QObject::deleteLater);
    return process;
}

void PackageKitNotifier::transactionListChanged(const QStringList& tids)
{
    if (PackageKit::Daemon::global()->offline()->updateTriggered())
        return;

    for (const auto &tid: tids) {
        if (m_transactions.contains(tid))
            continue;

        auto t = new PackageKit::Transaction(QDBusObjectPath(tid));

        connect(t, &PackageKit::Transaction::roleChanged, this, [this, t]() {
            if (t->role() == PackageKit::Transaction::RoleGetUpdates) {
                setupGetUpdatesTransaction(t);
            }
        });
        connect(t, &PackageKit::Transaction::requireRestart, this, &PackageKitNotifier::onRequireRestart);
        connect(t, &PackageKit::Transaction::finished, this, [this, t](){
            auto restart = t->property("requireRestart");
            if (!restart.isNull()) {
                auto restartEvent = PackageKit::Transaction::Restart(restart.toInt());
                if (restartEvent >= PackageKit::Transaction::RestartSession) {
                    nowNeedsReboot();
                }
            }
            m_transactions.remove(t->tid().path());
            t->deleteLater();
        });
        m_transactions.insert(tid, t);
    }
}

void PackageKitNotifier::nowNeedsReboot()
{
    if (!m_needsReboot) {
        m_needsReboot = true;
        Q_EMIT needsRebootChanged();
    }
}

void PackageKitNotifier::onRequireRestart(PackageKit::Transaction::Restart type, const QString &packageID)
{
    PackageKit::Transaction* t = qobject_cast<PackageKit::Transaction*>(sender());
    t->setProperty("requireRestart", qMax<int>(t->property("requireRestart").toInt(), type));
    qDebug() << "RESTART" << type << "is required for package" << packageID;
}
