/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "MuonNotifierModule.h"
#include "BackendNotifierFactory.h"
#include <KStatusNotifierItem>
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KLocalizedString>
#include <KIconLoader>
#include <KNotification>
#include <KPluginFactory>
#include <QMenu>
#include <QDBusInterface>
#include <QDBusReply>

K_PLUGIN_FACTORY(MuonNotifierDaemonFactory, registerPlugin<MuonNotifierModule>();)
K_EXPORT_PLUGIN(MuonNotifierDaemonFactory("muonnotifier", "muonnotifier"))

MuonNotifierModule::MuonNotifierModule(QObject * parent, const QVariantList& /*args*/)
    : KDEDModule(parent)
    , m_verbose(false)
{
    configurationChanged();

    m_statusNotifier = new KStatusNotifierItem("org.kde.muonnotifier", this);
    m_statusNotifier->setTitle(i18n("Muon Update Notifier"));
    m_statusNotifier->setIconByName("muondiscover");
    m_statusNotifier->setStandardActionsEnabled(false);
    m_statusNotifier->contextMenu()->clear();
    m_statusNotifier->contextMenu()->setTitle(i18n("Muon updates notifier"));
    m_statusNotifier->contextMenu()->setIcon(QIcon::fromTheme("muondiscover"));
    m_statusNotifier->contextMenu()->addAction(QIcon::fromTheme("muondiscover"), i18n("Open Muon..."), this, SLOT(showMuon()));
    m_statusNotifier->contextMenu()->addAction(QIcon::fromTheme("application-exit"), i18n("Quit notifier..."), this, SLOT(quit()));

    connect(m_statusNotifier, SIGNAL(activateRequested(bool, QPoint)), SLOT(showMuon()));

    m_backends = BackendNotifierFactory().allBackends();
    for(BackendNotifierModule* module : m_backends) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &MuonNotifierModule::updateStatusNotifier);
    }
    updateStatusNotifier();
}

MuonNotifierModule::~MuonNotifierModule()
{
}

void MuonNotifierModule::configurationChanged()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    m_verbose = notifyTypeGroup.readEntry("Verbose", false);
}

void MuonNotifierModule::showMuon()
{
    KRun::runCommand("muon-discover --mode installed", 0);
}

void MuonNotifierModule::quit()
{
    QDBusInterface kded("org.kde.kded", "/kded",
                        "org.kde.kded");
    kded.call("setModuleAutoloading", moduleName(), false);
    kded.call("unloadModule", moduleName());
}

bool MuonNotifierModule::isSystemUpToDate() const
{
    for(BackendNotifierModule* module : m_backends) {
        if(!module->isSystemUpToDate())
            return false;
    }
    return true;
}

void MuonNotifierModule::updateStatusNotifier()
{
    m_statusNotifier->setOverlayIconByName(iconName());
    if (!isSystemUpToDate()) {
        emit systemUpdateNeeded();
        //TODO: Better message strings
        QString msg = message();
        if (m_verbose) {
            msg += " " + extendedMessage();
        }
        m_statusNotifier->setToolTip(iconName(), msg, i18n("A system update is recommended"));
        m_statusNotifier->setStatus(KStatusNotifierItem::Active);

        KNotification::event("Update", i18n("System update available"), msg, QIcon::fromTheme("svn-update").pixmap(KIconLoader::SizeMedium), nullptr, KNotification::CloseOnTimeout, "muonabstractnotifier");
    } else {
        m_statusNotifier->setStatus(KStatusNotifierItem::Passive);
        m_statusNotifier->setToolTip(QString(), message(), i18n("Your system is up-to-date"));
    }
}

MuonNotifierModule::State MuonNotifierModule::state() const
{
    bool security = false, normal = false;

    for(BackendNotifierModule* module : m_backends) {
        security |= module->securityUpdatesCount()>0;
        normal |= security || module->updatesCount()>0;
        if (security)
            break;
    }
    if (security)
        return SecurityUpdates;
    else if (normal)
        return NormalUpdates;
    else
        return NoUpdates;
}

QString MuonNotifierModule::iconName() const
{
    switch(state()) {
        case SecurityUpdates:
            return QLatin1String("security-high");
        case NormalUpdates:
            return QLatin1String("security-low");
        case NoUpdates:
            return QLatin1String("security-high");
    }
    return QString();
}

QString MuonNotifierModule::message() const
{
    switch(state()) {
        case SecurityUpdates:
            return i18n("A security update is available for your system");
        case NormalUpdates:
            return i18n("An update is available for your system");
        case NoUpdates:
            return i18n("No system update available");
    }
    return QString();
}

QString MuonNotifierModule::extendedMessage() const
{
    int count = updatesCount(), securityCount = securityUpdatesCount();
    if (count > 0 && securityCount > 0) {
        return i18n("There are %1 updated packages, of which %2 were updated for security reasons", count, securityCount);
    } else if (count > 0) {
        return i18n("There are %1 updated packages", count);
    } else if (securityCount > 0) {
        return i18n("%1 packages were updated for security reasons", securityCount);
    } else {
        return i18n("System up to date");
    }
}

void MuonNotifierModule::recheckSystemUpdateNeeded()
{
    for(BackendNotifierModule* module : m_backends)
        module->recheckSystemUpdateNeeded();
}

int MuonNotifierModule::securityUpdatesCount() const
{
    int ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->securityUpdatesCount();
    return ret;
}

int MuonNotifierModule::updatesCount() const
{
    int ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->updatesCount();
    return ret;
}

QStringList MuonNotifierModule::loadedModules() const
{
    QStringList ret;
    for(BackendNotifierModule* module : m_backends)
        ret += module->metaObject()->className();
    return ret;
}

#include "MuonNotifierModule.moc"
