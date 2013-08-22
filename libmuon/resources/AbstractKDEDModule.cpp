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
#include <QProcess>

#include <KStatusNotifierItem>
#include <KLocale>
#include <KRun>
#include <KMenu>
#include <KIcon>

class AbstractKDEDModule::Private
{
public:
    Private(AbstractKDEDModule * par, const QString &n, const QString &i) 
      : q(par), name(n), iconName(i), systemUpToDate(true), updateType(AbstractKDEDModule::NormalUpdate), statusNotifier(0) {}
    ~Private() {}
    
    void __k__showMuon();
    void __k__quit();
    
    AbstractKDEDModule * q;
    QString name;
    QString iconName;
    bool systemUpToDate;
    AbstractKDEDModule::UpdateType updateType;
    KStatusNotifierItem * statusNotifier;
};

AbstractKDEDModule::AbstractKDEDModule(const QString &name, const QString &iconName, QObject * parent)
  : KDEDModule(parent),
    d(new Private(this, name, iconName))
{
    d->statusNotifier = new KStatusNotifierItem("org.kde.muon." + d->name, this);
    d->statusNotifier->setTitle(i18n("%1 update notifier", d->name));
    d->statusNotifier->setIconByName(iconName);
    d->statusNotifier->setStandardActionsEnabled(false);
    d->statusNotifier->contextMenu()->clear();
    d->statusNotifier->contextMenu()->addTitle(KIcon("svn-update"), i18n("Muon %1 update notifier", name));
    d->statusNotifier->contextMenu()->setIcon(KIcon(iconName));
    d->statusNotifier->contextMenu()->addAction(KIcon("muondiscover"), i18n("Open Muon..."), this, SLOT(__k__showMuon()));
    d->statusNotifier->contextMenu()->addAction(KIcon("application-exit"), i18n("Quit notifier..."), this, SLOT(__k__quit()));
    
    connect(d->statusNotifier, SIGNAL(activateRequested(bool, QPoint)), SLOT(__k__showMuon()));
}

AbstractKDEDModule::~AbstractKDEDModule()
{
    delete d;
}

void AbstractKDEDModule::Private::__k__showMuon()
{
    QProcess::execute("muon-discover --mode installed");//TODO: Move to KRun
}

void AbstractKDEDModule::Private::__k__quit()
{
    QDBusInterface kded("org.kde.kded", "/kded",      
                    "org.kde.kded");
    kded.call("setModuleAutoloading", q->moduleName(), false);//TODO: Change to abstract names
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

void AbstractKDEDModule::setSystemUpToDate(bool systemUpToDate)
{
    d->systemUpToDate = systemUpToDate;
    if (!systemUpToDate) {
        emit systemUpdateNeeded();
        //TODO: Better message strings
        if (d->updateType == SecurityUpdate) {
            d->statusNotifier->showMessage(i18n("System update available!"), i18n("A security update is available for your system!"), "svn-update", 1000);
            d->statusNotifier->setOverlayIconByName("security-low");
        } else {
            d->statusNotifier->showMessage(i18n("System update available!"), i18n("An update is available for your system!"), "svn-update", 1000);
            d->statusNotifier->setOverlayIconByName("security-high");
        }
        d->statusNotifier->setStatus(KStatusNotifierItem::Active);
    } else {
        d->statusNotifier->setOverlayIconByName(QString());
        d->statusNotifier->setStatus(KStatusNotifierItem::Passive);
    }
}

void AbstractKDEDModule::setUpdateType(int updateType)
{
    d->updateType = (UpdateType)updateType;
}

#include "AbstractKDEDModule.moc"
