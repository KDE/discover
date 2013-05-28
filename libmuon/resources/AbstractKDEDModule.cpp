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
    if (!QDBusConnection::systemBus().registerService("org.kde.muon." + d->name)) {
        qWarning() << "Cannot register service on session bus";
    }
    if (!QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAllContents)) {
        qWarning() << "Cannot register on session bus";
    }
    
    d->statusNotifier = new KStatusNotifierItem("org.kde.muon." + d->name, this);
    d->statusNotifier->setTitle(i18n("%1 update notifier", d->name));
    d->statusNotifier->setStandardActionsEnabled(false);//TODO: Add a "Quit" button to close the KDEModule
    d->statusNotifier->contextMenu()->addAction(KIcon("muondiscover"), i18n("Open Muon..."), this, SLOT(__k__showMuon()));
    connect(d->statusNotifier, SIGNAL(activateRequested(bool, QPoint)), SLOT(__k__showMuon()));
}

AbstractKDEDModule::~AbstractKDEDModule()
{
    delete d;
}

void AbstractKDEDModule::Private::__k__showMuon()
{
    QProcess::execute("muon-updater");//TODO: Move to KRun
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
    if (!systemUpToDate)
        emit systemUpdateNeeded();
}

void AbstractKDEDModule::setUpdateType(UpdateType updateType)
{
    d->updateType = updateType;
}

#include "AbstractKDEDModule.moc"
