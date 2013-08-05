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
#include <akabeiclient/akabeiclientbackend.h>
#include <akabeicore/akabeidatabase.h>
#include <akabeicore/akabeiconfig.h>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KDebug>

K_PLUGIN_FACTORY(MuonAkabeiBackendFactory, registerPlugin<AkabeiBackend>(); )
K_EXPORT_PLUGIN(MuonAkabeiBackendFactory(KAboutData("muon-akabeibackend","muon-akabeibackend",ki18n("Akabei Backend"),"0.1",ki18n("Chakra-Applications in your system"), KAboutData::License_GPL)))

AkabeiBackend::AkabeiBackend(QObject* parent, const QVariantList& ) : AbstractResourcesBackend(parent)
{
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
}

void AkabeiBackend::statusChanged(Akabei::Backend::Status status)
{
    kDebug() << "Status changed to" << status;
    if (status == Akabei::Backend::StatusReady && m_packages.isEmpty()) {
        emit backendReady();
        kDebug() << "get packages";
        connect(Akabei::Backend::instance(), SIGNAL(queryPackagesCompleted(QUuid,QList<Akabei::Package*>)), SLOT(queryComplete(QUuid,QList<Akabei::Package*>)));
        Akabei::Backend::instance()->packages();
    }
}

void AkabeiBackend::queryComplete(QUuid,QList<Akabei::Package*> packages)
{
    emit reloadStarted();
    kDebug() << "Got" << packages.count() << "packages";
    foreach (Akabei::Package * pkg, packages) {
        if (m_packages.contains(pkg->name())) {
            qobject_cast<AkabeiResource*>(m_packages[pkg->name()])->addPackage(pkg);
        } else {
            m_packages.insert(pkg->name(), new AkabeiResource(pkg, this));
        }
    }
    emit reloadFinished();
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
    installApplication(app);
}

void AkabeiBackend::installApplication(AbstractResource* app)
{
}

void AkabeiBackend::removeApplication(AbstractResource* app)
{

}

void AkabeiBackend::cancelTransaction(AbstractResource* app)
{

}

AbstractBackendUpdater* AkabeiBackend::backendUpdater() const
{
    return nullptr;
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

