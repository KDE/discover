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
#include "AbstractKDEDModule.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusreply.h>
#include <QDebug>

#include <KStatusNotifierItem>
#include <KRun>
#include <QMenu>
#include <KNotification>
#include <KIconLoader>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

class AbstractKDEDModule::Private
{
public:
    Private(AbstractKDEDModule * par, const QString &n, const QString &i) 
      : q(par), 
        name(n), 
        iconName(i), 
        systemUpToDate(true), 
        updateType(AbstractKDEDModule::NormalUpdate), 
        statusNotifier(0),
        updateCount(0),
        securityUpdateCount(0) {}
    ~Private() {}
    
    void __k__showMuon();
    void __k__quit();
    
    AbstractKDEDModule * q;
    QString name;
    QString iconName;
    bool systemUpToDate;
    AbstractKDEDModule::UpdateType updateType;
    KStatusNotifierItem * statusNotifier;
    bool verbose;
    int updateCount;
    int securityUpdateCount;
};

AbstractKDEDModule::AbstractKDEDModule(const QString &name, const QString &iconName, QObject * parent)
  : KDEDModule(parent),
    d(new Private(this, name, iconName))
{
    configurationChanged();
    
    d->statusNotifier = new KStatusNotifierItem("org.kde.muon." + d->name, this);
    d->statusNotifier->setTitle(i18n("%1 update notifier", d->name));
    d->statusNotifier->setIconByName(iconName);
    d->statusNotifier->setStandardActionsEnabled(false);
    d->statusNotifier->contextMenu()->clear();
    d->statusNotifier->contextMenu()->setTitle(i18n("Muon %1 update notifier", name));
    d->statusNotifier->contextMenu()->setIcon(QIcon::fromTheme(iconName));
    d->statusNotifier->contextMenu()->addAction(QIcon::fromTheme("muondiscover"), i18n("Open Muon..."), this, SLOT(__k__showMuon()));
    d->statusNotifier->contextMenu()->addAction(QIcon::fromTheme("application-exit"), i18n("Quit notifier..."), this, SLOT(__k__quit()));
    
    connect(d->statusNotifier, SIGNAL(activateRequested(bool, QPoint)), SLOT(__k__showMuon()));
}

AbstractKDEDModule::~AbstractKDEDModule()
{
    delete d;
}

void AbstractKDEDModule::configurationChanged()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    d->verbose = notifyTypeGroup.readEntry("Verbose", false);
}

void AbstractKDEDModule::Private::__k__showMuon()
{
    KRun::runCommand("muon-discover --mode installed", 0);
}

void AbstractKDEDModule::Private::__k__quit()
{
    QDBusInterface kded("org.kde.kded", "/kded",
                    "org.kde.kded");
    kded.call("setModuleAutoloading", q->moduleName(), false);
    QDBusReply<bool> reply = kded.call("unloadModule", q->moduleName());
}

bool AbstractKDEDModule::isSystemUpToDate() const
{
    return d->systemUpToDate;
}

int AbstractKDEDModule::updateType() const
{
    return d->updateType;
}

void AbstractKDEDModule::setSystemUpToDate(bool systemUpToDate, UpdateType updateType, Notification notification)
{
    setSystemUpToDate(systemUpToDate, -1, -1, updateType, notification);
}

void AbstractKDEDModule::setSystemUpToDate(bool systemUpToDate, int updateCount, UpdateType updateType, Notification notification)
{
    setSystemUpToDate(systemUpToDate, updateCount, -1, updateType, notification);
}

void AbstractKDEDModule::setSystemUpToDate(bool systemUpToDate, int updateCount, int securityUpdateCount, UpdateType updateType, Notification notification)
{
    if (!systemUpToDate) {
        emit systemUpdateNeeded();
        //TODO: Better message strings
        QString message;
        QString icon;

        if (updateType == SecurityUpdate) {
            message = i18n("A security update is available for your system.");
            icon = "security-low";
        } else {
            message = i18n("An update is available for your system!");
            icon = "security-high";
        }
        if (d->verbose) {
            if (updateCount > 0 && securityUpdateCount > 0) {
                message += " " + i18n("There are %1 updated packages, of which %2 were updated for security reasons!", updateCount, securityUpdateCount);
            } else if (updateCount > 0) {
                message += " " + i18n("There are %1 updated packages!", updateCount);
            } else if (securityUpdateCount > 0) {
                message += " " + i18n("%1 packages were updated for security reasons", securityUpdateCount);
            }
        }
        d->statusNotifier->setOverlayIconByName(icon);
        d->statusNotifier->setToolTip(icon, message, i18n("A system update is recommended"));
        d->statusNotifier->setStatus(KStatusNotifierItem::Active);
        if (notification == ShowNotification || 
	    (notification == ShowNotificationIfInformationChanged 
	     && (d->updateCount != updateCount 
	         || d->securityUpdateCount != securityUpdateCount
	         || d->updateType != updateType))) {
            KNotification::event("Update", i18n("System update available"), message, QIcon::fromTheme("svn-update").pixmap(KIconLoader::SizeMedium), nullptr, KNotification::CloseOnTimeout, "muonabstractnotifier");
        }
    } else {
        d->statusNotifier->setOverlayIconByName(QString());
        d->statusNotifier->setStatus(KStatusNotifierItem::Passive);
        d->statusNotifier->setToolTip("security-high", i18n("Your system is up-to-date!"), i18n("No system update available"));
    }
    d->updateCount = updateCount;
    d->securityUpdateCount = securityUpdateCount;
    d->systemUpToDate = systemUpToDate;
    d->updateType = updateType;
}

#include "moc_AbstractKDEDModule.cpp"
