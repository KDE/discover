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

// KDE includes
#include <KAboutData>
#include <KDirWatch>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KProcess>
#include <KStandardDirs>
#include <KNotification>
#include <KIcon>
#include <KIconLoader>

// Own includes

K_PLUGIN_FACTORY(ApplicationNotifierFactory,
                 registerPlugin<ApplicationNotifier>();
                )
K_EXPORT_PLUGIN(ApplicationNotifierFactory("muon-application-notifier"))

ApplicationNotifier::ApplicationNotifier(QObject* parent, const QVariantList &)
  : AbstractKDEDModule("Application", "muondiscover", parent), 
    m_checkerProcess(0), m_updateCheckerProcess(0), m_checkingForUpdates(false)
{
    KAboutData aboutData("muonapplicationnotifier", "muonapplicationnotifier",
                         ki18n("Muon Notification Daemon"),
                         "2.0", ki18n("A Notification Daemon for Muon"),
                         KAboutData::License_GPL,
                         ki18n("(C) 2013 Lukas Appelhans, (C) 2009-2012 Jonathan Thomas, (C) 2009 Harald Sitter"),
                         KLocalizedString(), "http://kubuntu.org");
    
    QTimer::singleShot(2 * 60 * 1000, this, SLOT(init()));
}

ApplicationNotifier::~ApplicationNotifier()
{
    delete m_checkerProcess;
    delete m_updateCheckerProcess;
}

void ApplicationNotifier::init()
{
    KDirWatch *stampDirWatch = new KDirWatch(this);
    stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
    connect(stampDirWatch, SIGNAL(dirty(QString)),
             this, SLOT(distUpgradeEvent()));
    
    distUpgradeEvent();

    stampDirWatch = new KDirWatch(this);
    stampDirWatch->addDir("/var/lib/apt/lists/");
    stampDirWatch->addDir("/var/lib/apt/lists/partial/");
    stampDirWatch->addFile("/var/lib/update-notifier/updates-available");
    stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
    connect(stampDirWatch, SIGNAL(dirty(QString)),
            this, SLOT(recheckSystemUpdateNeeded()));

    recheckSystemUpdateNeeded();
}

void ApplicationNotifier::distUpgradeEvent()
{
    QString checkerFile = KStandardDirs::locate("data", "muonapplicationnotifier/releasechecker");
    qDebug() << "Run releasechecker: " << checkerFile;
    m_checkerProcess = new KProcess(this);
    connect(m_checkerProcess, SIGNAL(finished(int)),
            this, SLOT(checkUpgradeFinished(int)));
    m_checkerProcess->setProgram(QStringList() << "/usr/bin/python3" << checkerFile);
    m_checkerProcess->start();
}

void ApplicationNotifier::checkUpgradeFinished(int exitStatus)
{
    qWarning() << "checked for upgrades and return with " << exitStatus;
    if (exitStatus == 0) {
        KNotification::event("DistUpgrade", i18n("System update available!"), i18nc("Notification when a new version of Kubuntu is available",
                                 "A new version of Kubuntu is available"), KIcon("svn-update").pixmap(KIconLoader::SizeMedium), nullptr, KNotification::CloseOnTimeout, KComponentData("muonapplicationnotifier"));
        setSystemUpToDate(false, AbstractKDEDModule::NormalUpdate, AbstractKDEDModule::DontShowNotification);
    }

    m_checkerProcess->deleteLater();
    m_checkerProcess = nullptr;
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
    int securityUpdates = 0;
    int updates = 0;
    // Weirdly enough, apt-check gives output on stderr
    QByteArray line = m_updateCheckerProcess->readAllStandardError();
    m_updateCheckerProcess->deleteLater();
    m_updateCheckerProcess = nullptr;

    // Format updates;security
    int eqpos = line.indexOf(';');

    if (eqpos > 0) {
        QByteArray updatesString = line.left(eqpos);
        QByteArray securityString = line.right(line.size() - eqpos - 1);
        
        securityUpdates = securityString.toInt();
        updates = updatesString.toInt() - securityUpdates;
    }

    // ';' not found, apt-check broke :("
    
    if (QFile::exists("/var/lib/update-notifier/updates-available")) {
        setSystemUpToDate(false, updates, securityUpdates, securityUpdates > 0 ? AbstractKDEDModule::SecurityUpdate : NormalUpdate);
    } else {
        setSystemUpToDate(true);
    }
    
    m_checkingForUpdates = false;
}

