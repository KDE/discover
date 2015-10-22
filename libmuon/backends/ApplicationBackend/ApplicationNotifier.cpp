/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
#include "ApplicationNotifier.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>
#include <QtCore/QProcess>
#include <QtGui/QIcon>

// KDE includes
#include <KDirWatch>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KNotification>
#include <KIconLoader>

ApplicationNotifier::ApplicationNotifier(QObject* parent)
  : BackendNotifierModule(parent)
  , m_checkerProcess(Q_NULLPTR)
  , m_updateCheckerProcess(Q_NULLPTR)
  , m_checkingForUpdates(false)
  , m_securityUpdates(0)
  , m_normalUpdates(0)
{
    KDirWatch *stampDirWatch = new KDirWatch(this);
    stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
    connect(stampDirWatch, &KDirWatch::dirty, this, &ApplicationNotifier::distUpgradeEvent);

    stampDirWatch = new KDirWatch(this);
    stampDirWatch->addDir("/var/lib/apt/lists/");
    stampDirWatch->addDir("/var/lib/apt/lists/partial/");
    stampDirWatch->addFile("/var/lib/update-notifier/updates-available");
    stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
    connect(stampDirWatch, &KDirWatch::dirty, this, &ApplicationNotifier::recheckSystemUpdateNeeded);

    //check in 2 minutes
    QTimer::singleShot(2 * 60 * 1000, this, &ApplicationNotifier::recheckSystemUpdateNeeded);

    init();
}

ApplicationNotifier::~ApplicationNotifier()
{
}

void ApplicationNotifier::init()
{
    recheckSystemUpdateNeeded();
    distUpgradeEvent();
}

void ApplicationNotifier::distUpgradeEvent()
{
    QString checkerFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "libmuon/applicationsbackend/releasechecker");
    if (checkerFile.isEmpty()) {
        qWarning() << "Couldn't find the releasechecker" << checkerFile << QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        return;
    }
    m_checkerProcess = new QProcess(this);
    connect(m_checkerProcess, SIGNAL(finished(int)),
            this, SLOT(checkUpgradeFinished(int)));
    m_checkerProcess->start("/usr/bin/python3", QStringList() << checkerFile);
}

void ApplicationNotifier::checkUpgradeFinished(int exitStatus)
{
    if (exitStatus == 0) {
        KNotification *n = KNotification::event("DistUpgrade",
                                                i18n("System update available!"),
                                                i18nc("Notification when a new version of Kubuntu is available",
                                                      "A new version of Kubuntu is available"),
                                                QStringLiteral("system-software-update"),
                                                nullptr,
                                                KNotification::CloseOnTimeout,
                                                "muonapplicationnotifier");
        n->setActions(QStringList() << i18n("Upgrade"));
        connect(n, &KNotification::action1Activated, this, &ApplicationNotifier::upgradeActivated);
    }

    m_checkerProcess->deleteLater();
    m_checkerProcess = nullptr;
}

void ApplicationNotifier::upgradeActivated()
{
    const QString kdesu = QFile::decodeName(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/kdesu");
    QProcess::startDetached(kdesu, { "--", "do-release-upgrade", "-m", "desktop", "-f", "DistUpgradeViewKDE" });
}

void ApplicationNotifier::recheckSystemUpdateNeeded()
{
    if (m_checkingForUpdates)
        return;
    
    m_checkingForUpdates = true;
    m_updateCheckerProcess = new QProcess(this);
    connect(m_updateCheckerProcess, SIGNAL(finished(int)), this, SLOT(parseUpdateInfo()));
    m_updateCheckerProcess->start("/usr/lib/update-notifier/apt-check");
}
    
void ApplicationNotifier::parseUpdateInfo()
{
#warning why does this parse stdout and not use qapt, wtf...
    m_securityUpdates = 0;
    m_normalUpdates = 0;
    // Weirdly enough, apt-check gives output on stderr
    QByteArray line = m_updateCheckerProcess->readAllStandardError();
    m_updateCheckerProcess->deleteLater();
    m_updateCheckerProcess = nullptr;

    // Format updates;security
    int eqpos = line.indexOf(';');

    if (eqpos > 0) {
        QByteArray updatesString = line.left(eqpos);
        QByteArray securityString = line.right(line.size() - eqpos - 1);
        
        int securityUpdates = securityString.toInt();
        setUpdates(updatesString.toInt() - securityUpdates, securityUpdates);
    }
    
    m_checkingForUpdates = false;
}

void ApplicationNotifier::setUpdates(int normal, int security)
{
    if (m_normalUpdates != normal || security != m_securityUpdates) {
        m_normalUpdates = normal;
        m_securityUpdates = security;
        emit foundUpdates();
    }
}

bool ApplicationNotifier::isSystemUpToDate() const
{
    return (m_securityUpdates+m_normalUpdates)==0;
}

uint ApplicationNotifier::securityUpdatesCount()
{
    return m_securityUpdates;
}

uint ApplicationNotifier::updatesCount()
{
    return m_normalUpdates;
}

#include "ApplicationNotifier.moc"
