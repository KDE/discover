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
    Private(const QString &n) 
      : name(n), systemUpToDate(true), updateType(AbstractKDEDModule::NormalUpdate), statusNotifier(0) {}
    ~Private() {}
    
    void __k__showMuon();
    
    QString name;
    bool systemUpToDate;
    AbstractKDEDModule::UpdateType updateType;
    KStatusNotifierItem * statusNotifier;
};

AbstractKDEDModule::AbstractKDEDModule(const QString &name, QObject * parent)
  : KDEDModule(parent),
    d(new Private(name))
{
    d->statusNotifier = new KStatusNotifierItem("org.kde.muon." + d->name, this);
    d->statusNotifier->setTitle(i18n("%1 update notifier", d->name));
    d->statusNotifier->setIconByName("muondiscover");
    d->statusNotifier->setStandardActionsEnabled(false);//TODO: Add a "Quit" button to close the KDEModule
    d->statusNotifier->contextMenu()->clear();
    d->statusNotifier->contextMenu()->addTitle(KIcon("svn-update"), i18n("Muon %1 update notifier", name));
    d->statusNotifier->contextMenu()->setIcon(KIcon("muondiscover"));
    d->statusNotifier->contextMenu()->addAction(KIcon("muondiscover"), i18n("Open Muon..."), this, SLOT(__k__showMuon()));
    
    if (!QDBusConnection::sessionBus().registerService("org.kde.muon." + d->name)) {
        d->statusNotifier->showMessage(i18n("DBus failure"), i18n("Cannot register service on system bus!"), "muondiscover");
        qWarning() << "Cannot register service on session bus";
    }
    //TODO: Probably don't export so much (don't export KDED stuff etc)
    if (!QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAllContents)) {
        d->statusNotifier->showMessage(i18n("DBus failure"), i18n("Cannot register object on system bus!"), "muondiscover");
        qWarning() << "Cannot register on session bus";
    }
    
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

bool AbstractKDEDModule::isSystemUpToDate() const
{
    return d->systemUpToDate;
}

AbstractKDEDModule::UpdateType AbstractKDEDModule::updateType() const
{
    return d->updateType;
}

void AbstractKDEDModule::setSystemUpToDate(bool systemUpToDate)
{
    d->systemUpToDate = systemUpToDate;
    if (!systemUpToDate) {
        emit systemUpdateNeeded();
        //TODO: Better message strings
        d->statusNotifier->showMessage(i18n("System update available!"), i18n("The system is not up-to-date!"), "svn-update", 2000);
        d->statusNotifier->setOverlayIconByName("svn-update");
        d->statusNotifier->setStatus(KStatusNotifierItem::Active);
    } else {
        d->statusNotifier->setOverlayIconByName(QString());
        d->statusNotifier->setStatus(KStatusNotifierItem::Passive);
    }
}

void AbstractKDEDModule::setUpdateType(UpdateType updateType)
{
    d->updateType = updateType;
}

#include "AbstractKDEDModule.moc"
