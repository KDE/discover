/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <boom1992@chakra-project.org>        *
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
#include "AkabeiBackend.h"
#include "AkabeiResource.h"
#include "AkabeiTransaction.h"
#include "AppAkabeiResource.h"
#include "AppstreamUtils.h"
#include <Transaction/TransactionModel.h>
#include <akabeiclient/akabeiclientbackend.h>
#include <akabeicore/akabeidatabase.h>
#include <akabeicore/akabeiconfig.h>

#include <QQueue>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KDebug>

K_PLUGIN_FACTORY(MuonAkabeiBackendFactory, registerPlugin<AkabeiBackend>(); )
K_EXPORT_PLUGIN(MuonAkabeiBackendFactory(KAboutData("muon-akabeibackend","muon-akabeibackend",ki18n("Akabei Backend"),"0.1",ki18n("Chakra-Applications in your system"), KAboutData::License_GPL)))

AkabeiBackend::AkabeiBackend(QObject* parent, const QVariantList& ) : AbstractResourcesBackend(parent), m_updater(new AkabeiUpdater(this))
{
    m_transactionQueue.clear();
    kDebug() << "CONSTRUCTED";
    connect(AkabeiClient::Backend::instance(), SIGNAL(statusChanged(Akabei::Backend::Status)), SLOT(statusChanged(Akabei::Backend::Status)));
    
    /* Used to determine whether debugging prints are to be displayed later */
    Akabei::Config::instance()->setDebug(true);

    QLocale systemLocale = QLocale::system();
    AkabeiClient::Backend::instance()->setLocale( systemLocale.name() );
    AkabeiClient::Backend::instance()->initialize();
}

AkabeiBackend::~AkabeiBackend()
{
    Akabei::Backend::instance()->deInit();
}

void AkabeiBackend::statusChanged(Akabei::Backend::Status status)
{
    kDebug() << "Status changed to" << status;
    if (status == Akabei::Backend::StatusReady && m_packages.isEmpty()) {
        emit backendReady();
        reload();
    }
}

void AkabeiBackend::reload()
{
    m_appdata = AppstreamUtils::fetchAppData("/usr/share/app-info/appdata.xml");
    kDebug() << "get packages";
    connect(Akabei::Backend::instance(), SIGNAL(queryPackagesCompleted(QUuid,QList<Akabei::Package*>)), SLOT(queryComplete(QUuid,QList<Akabei::Package*>)));
    Akabei::Backend::instance()->packages();
}

void AkabeiBackend::queryComplete(QUuid,QList<Akabei::Package*> packages)
{
    disconnect(Akabei::Backend::instance(), SIGNAL(queryPackagesCompleted(QUuid,QList<Akabei::Package*>)), this, SLOT(queryComplete(QUuid,QList<Akabei::Package*>)));
    kDebug() << "Got" << packages.count() << "packages";
    emit reloadStarted();
    QHash<QString, AbstractResource*> pkgs;
    foreach (Akabei::Package * pkg, packages) {
        if (pkgs.contains(pkg->name())) {
            qobject_cast<AkabeiResource*>(pkgs[pkg->name()])->addPackage(pkg);
        } else if (m_packages.contains(pkg->name())) {
            AkabeiResource * res = qobject_cast<AkabeiResource*>(m_packages[pkg->name()]);
            res->clearPackages();
            res->addPackage(pkg);
            pkgs.insert(pkg->name(), res);
        } else {
            if (m_appdata.contains(pkg->name())) {
                pkgs.insert(pkg->name(), new AppAkabeiResource(m_appdata[pkg->name()], pkg, this));
            } else {
                pkgs.insert(pkg->name(), new AkabeiResource(pkg, this));
            }
        }
    }
    m_packages = pkgs;
    emit reloadFinished();
    if (m_transactionQueue.count() >= 1) {
        AkabeiTransaction * trans = m_transactionQueue.first();
        trans->start();
    }
}

bool AkabeiBackend::isValid() const
{
    return Akabei::Backend::instance()->status() != Akabei::Backend::StatusBroken;
}

AbstractReviewsBackend* AkabeiBackend::reviewsBackend() const
{
    return nullptr;
}

AbstractResource* AkabeiBackend::resourceByPackageName(const QString& name) const
{
    return m_packages[name];
}

int AkabeiBackend::updatesCount() const
{
    int count = 0;
    for (AbstractResource * res : m_packages.values()) {
        if (!res->isTechnical() && res->canUpgrade())
            count++;
    }
    return count;
}

QVector< AbstractResource* > AkabeiBackend::allResources() const
{
    return m_packages.values().toVector();
}

QList< AbstractResource* > AkabeiBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> result;
    for (AbstractResource * res : m_packages.values()) {
        if (res->name().contains(searchText, Qt::CaseInsensitive) || res->comment().contains(searchText, Qt::CaseInsensitive))
            result << res;
    }
    return result;
}

void AkabeiBackend::installApplication(AbstractResource* app, AddonList addons)
{
    Transaction::Role role = Transaction::InstallRole;
    if (app->isInstalled() && !app->canUpgrade()) {
        role = Transaction::ChangeAddonsRole;
    }
    AkabeiTransaction * trans = new AkabeiTransaction(this, app, role, addons);
    TransactionModel::global()->addTransaction(trans);
    m_transactionQueue.enqueue(trans);
    if (m_transactionQueue.count() <= 1)
        trans->start();
}

void AkabeiBackend::installApplication(AbstractResource* app)
{
    AkabeiTransaction * trans = new AkabeiTransaction(this, app, Transaction::InstallRole);
    TransactionModel::global()->addTransaction(trans);
    m_transactionQueue.enqueue(trans);
    if (m_transactionQueue.count() <= 1)
        trans->start();
}

void AkabeiBackend::removeApplication(AbstractResource* app)
{    
    AkabeiTransaction * trans = new AkabeiTransaction(this, app, Transaction::RemoveRole);
    TransactionModel::global()->addTransaction(trans);
    m_transactionQueue.enqueue(trans);
    if (m_transactionQueue.count() <= 1)
        trans->start();
}

void AkabeiBackend::removeFromQueue(AkabeiTransaction* trans)
{
    m_transactionQueue.removeAll(trans);
    TransactionModel::global()->removeTransaction(trans);
    reload();
}

void AkabeiBackend::cancelTransaction(AbstractResource* app)
{

}

AbstractBackendUpdater* AkabeiBackend::backendUpdater() const
{
    return m_updater;
}

QList< AbstractResource* > AkabeiBackend::upgradeablePackages() const
{
    QList<AbstractResource*> resources;
    for (AbstractResource * res : m_packages.values()) {
        if (!res->isTechnical() && res->canUpgrade())
            resources << res;
    }
    return resources;
}

