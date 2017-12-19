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
#include <QDBusInterface>
#include <KLocalizedString>

PackageKitNotifier::PackageKitNotifier(QObject* parent)
    : BackendNotifierModule(parent)
    , m_securityUpdates(0)
    , m_normalUpdates(0)
{
    if (PackageKit::Daemon::global()->isRunning()) {
        recheckSystemUpdateNeeded();
    }
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::networkStateChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::transactionListChanged, this, &PackageKitNotifier::transactionListChanged);

    //Check if there's packages after 5'
    QTimer::singleShot(5 * 60 * 1000, this, &PackageKitNotifier::refreshDatabase);

    QTimer *regularCheck = new QTimer(this);
    regularCheck->setInterval(24 * 60 * 60 * 1000); //refresh at least once every day
    connect(regularCheck, &QTimer::timeout, this, &PackageKitNotifier::refreshDatabase);

    const QString aptconfig = QStandardPaths::findExecutable(QStringLiteral("apt-config"));
    if (!aptconfig.isEmpty()) {
        auto process = checkAptVariable(aptconfig, QLatin1String("Apt::Periodic::Update-Package-Lists"), [regularCheck](const QStringRef& value) {
            bool ok;
            int time = value.toInt(&ok);
            if (ok && time > 0)
                regularCheck->setInterval(time * 60 * 60 * 1000);
            else
                qWarning() << "couldn't understand value for timer:" << value;
        });
        connect(process, static_cast<void(QProcess::*)(int)>(&QProcess::finished), regularCheck, static_cast<void(QTimer::*)()>(&QTimer::start));
    } else
        regularCheck->start();
}

PackageKitNotifier::~PackageKitNotifier()
{
}

void PackageKitNotifier::recheckSystemUpdateNeeded()
{
    if (PackageKit::Daemon::global()->isRunning()) {
        PackageKit::Transaction * trans = PackageKit::Daemon::getUpdates();
        trans->setProperty("normalUpdates", 0);
        trans->setProperty("securityUpdates", 0);
        connect(trans, &PackageKit::Transaction::package, this, &PackageKitNotifier::package);
        connect(trans, &PackageKit::Transaction::finished, this, &PackageKitNotifier::finished);
    }
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

bool PackageKitNotifier::isSystemUpToDate() const
{
    return m_securityUpdates == 0 && m_normalUpdates == 0;
}

uint PackageKitNotifier::securityUpdatesCount()
{
    return m_securityUpdates;
}

uint PackageKitNotifier::updatesCount()
{
    return m_normalUpdates;
}

void PackageKitNotifier::onDistroUpgrade(PackageKit::Transaction::DistroUpgrade type, const QString& name, const QString& description)
{
    KNotification *notification = new KNotification(QLatin1String("distupgrade-notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setActions(QStringList{QLatin1String("Upgrade")});
    notification->setTitle(i18n("Upgrade available"));
    switch(type) {
        case PackageKit::Transaction::DistroUpgradeUnknown:
        case PackageKit::Transaction::DistroUpgradeUnstable:
            notification->setText(i18n("New unstable version: %1", description));
            break;
        case PackageKit::Transaction::DistroUpgradeStable:
            notification->setText(i18n("New version: %1", description));
            break;
    }

    connect(notification, &KNotification::action1Activated, this, [name] () {
        PackageKit::Daemon::upgradeSystem(name, PackageKit::Transaction::UpgradeKindDefault);
    });

    notification->sendEvent();
}

void PackageKitNotifier::refreshDatabase()
{
    if (!m_refresher) {
        m_refresher = PackageKit::Daemon::refreshCache(false);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, [this]() {
            recheckSystemUpdateNeeded();
            delete m_refresher;
        });
    }

    if (!m_distUpgrades && (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleUpgradeSystem)) {
        m_distUpgrades = PackageKit::Daemon::getDistroUpgrades();
        connect(m_distUpgrades, &PackageKit::Transaction::distroUpgrade, this, &PackageKitNotifier::onDistroUpgrade);
        connect(m_distUpgrades.data(), &PackageKit::Transaction::finished, this, [this]() {
            recheckSystemUpdateNeeded();
            delete m_distUpgrades;
        });
    }
}

QProcess* PackageKitNotifier::checkAptVariable(const QString &aptconfig, const QLatin1String& varname, std::function<void(const QStringRef& val)> func)
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
            }
        }
    });
    connect(process, static_cast<void(QProcess::*)(int)>(&QProcess::finished), process, &QObject::deleteLater);
    return process;
}

void PackageKitNotifier::transactionListChanged(const QStringList& tids)
{
    for (const auto &tid: tids) {
        if (m_transactions.contains(tid))
            continue;

        auto t = new PackageKit::Transaction(QDBusObjectPath(tid));
        connect(t, &PackageKit::Transaction::requireRestart, this, &PackageKitNotifier::onRequireRestart);
        connect(t, &PackageKit::Transaction::finished, this, [this, t](){
            auto restart = t->property("requireRestart");
            if (!restart.isNull())
                requireRestartNotification(PackageKit::Transaction::Restart(restart.toInt()));
            m_transactions.remove(t->tid().path());
            t->deleteLater();
        });
        m_transactions.insert(tid, t);
    }
}

void PackageKitNotifier::onRequireRestart(PackageKit::Transaction::Restart type, const QString &packageID)
{
    PackageKit::Transaction* t = qobject_cast<PackageKit::Transaction*>(sender());
    t->setProperty("requireRestart", qMax<int>(t->property("requireRestart").toInt(), type));
    qDebug() << "RESTART" << type << "is required for package" << packageID;
}

void PackageKitNotifier::requireRestartNotification(PackageKit::Transaction::Restart type)
{
    if (type < PackageKit::Transaction::RestartSession) {
        return;
    }

    KNotification *notification = new KNotification(QLatin1String("notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    if (type == PackageKit::Transaction::RestartSystem || type == PackageKit::Transaction::RestartSecuritySystem) {
        notification->setActions(QStringList{QLatin1String("Restart")});
        notification->setTitle(i18n("Restart is required"));
        notification->setText(i18n("The system needs to be restarted for the updates to take effect."));
    } else {
        notification->setActions(QStringList{QLatin1String("Logout")});
        notification->setTitle(i18n("Session restart is required"));
        notification->setText(i18n("You will need to log out and back in for the update to take effect."));
    }

    connect(notification, &KNotification::action1Activated, this, [type] () {
        QDBusInterface interface(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QStringLiteral("org.kde.KSMServerInterface"), QDBusConnection::sessionBus());
        if (type == PackageKit::Transaction::RestartSystem) {
            interface.asyncCall(QStringLiteral("logout"), 0, 1, 2); // Options: do not ask again | reboot | force
        } else {
            interface.asyncCall(QStringLiteral("logout"), 0, 0, 2); // Options: do not ask again | logout | force
        }
    });

    notification->sendEvent();
}
