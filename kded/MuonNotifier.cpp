/***************************************************************************
 *   Copyright © 2009 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2009 Harald Sitter <apachelogger@ubuntu.com>              *
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

#include "MuonNotifier.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QTimer>

// KDE includes
#include <KAboutData>
#include <KDirWatch>
#include <KLocalizedString>
#include <KPluginFactory>

// Own includes
#include "distupgradeevent/distupgradeevent.h"
#include "UpdateEvent/UpdateEvent.h"

#include "configwatcher.h"

K_PLUGIN_FACTORY(MuonNotifierFactory,
                 registerPlugin<MuonNotifier>();
                )
K_EXPORT_PLUGIN(MuonNotifierFactory("muon-notifier"))


MuonNotifier::MuonNotifier(QObject* parent, const QList<QVariant>&)
        : KDEDModule(parent)
        , m_distUpgradeEvent(0)
        , m_configWatcher(0)
{
    KAboutData aboutData("muon-notifier", "muon-notifier",
                         ki18n("Muon Notification Daemon"),
                         "1.2", ki18n("A Notification Daemon for Muon"),
                         KAboutData::License_GPL,
                         ki18n("(C) 2009-2011 Jonathan Thomas, (C) 2009 Harald Sitter"),
                         KLocalizedString(), "http://kubuntu.org");

    QTimer::singleShot(0, this, SLOT(init()));
}

MuonNotifier::~MuonNotifier()
{
}

void MuonNotifier::init()
{
    m_configWatcher = new ConfigWatcher(this);

    m_distUpgradeEvent = new DistUpgradeEvent(this, "DistUpgrade");
    m_updateEvent = new UpdateEvent(this, "Update");

    if (!m_distUpgradeEvent->isHidden()) {
        KDirWatch *stampDirWatch = new KDirWatch(this);
        stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
        connect(stampDirWatch, SIGNAL(dirty(const QString &)),
                this, SLOT(distUpgradeEvent()));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_distUpgradeEvent, SLOT(reloadConfig()));

        distUpgradeEvent();
    }

    if (!m_updateEvent->isHidden()) {
        KDirWatch *stampDirWatch = new KDirWatch(this);
        stampDirWatch->addDir("/var/lib/apt/lists/");
        stampDirWatch->addDir("/var/lib/apt/lists/partial/");
        stampDirWatch->addFile("/var/lib/update-notifier/updates-available");
        connect(stampDirWatch, SIGNAL(dirty(const QString &)),
                this, SLOT(updateEvent()));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_updateEvent, SLOT(reloadConfig()));

        updateEvent();
    }
}

void MuonNotifier::distUpgradeEvent()
{
    m_distUpgradeEvent->show();
}

void MuonNotifier::updateEvent()
{
    if (QFile::exists("/var/lib/update-notifier/updates-available")) {
        m_updateEvent->getUpdateInfo();
    }
}

#include "MuonNotifier.moc"
