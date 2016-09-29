/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PKTransaction.h"
#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include "PackageKitMessages.h"
#include <resources/AbstractResource.h>
#include <Transaction/TransactionModel.h>
#include <QDebug>
#include <QMessageBox>
#include <KLocalizedString>
#include <PackageKit/Transaction>
#include <PackageKit/Daemon>
#include <functional>

PKTransaction::PKTransaction(const QVector<AbstractResource*>& apps, Transaction::Role role)
    : Transaction(apps.first(), apps.first(), role)
    , m_apps(apps)
{
    Q_ASSERT(!apps.contains(nullptr));
    foreach(auto r, apps) {
        PackageKitResource* res = qobject_cast<PackageKitResource*>(r);
        m_pkgnames.unite(res->allPackageNames().toSet());
    }
}

static QStringList packageIds(const QVector<AbstractResource*>& res, std::function<QString(PackageKitResource*)> func)
{
    QStringList ret;
    foreach(auto r, res) {
        ret += func(qobject_cast<PackageKitResource*>(r));
    }
    ret.removeDuplicates();
    return ret;
}

void PKTransaction::start()
{
    trigger(PackageKit::Transaction::TransactionFlagSimulate);
}

void PKTransaction::trigger(PackageKit::Transaction::TransactionFlags flags)
{
    if (m_trans)
        m_trans->deleteLater();
    m_newPackageStates.clear();

    switch (role()) {
        case Transaction::ChangeAddonsRole:
        case Transaction::InstallRole:
            m_trans = PackageKit::Daemon::installPackages(packageIds(m_apps, [](PackageKitResource* r){return r->availablePackageId(); }), flags);
            break;
        case Transaction::RemoveRole:
            //see bug #315063
            m_trans = PackageKit::Daemon::removePackages(packageIds(m_apps, [](PackageKitResource* r){return r->installedPackageId(); }), true /*allowDeps*/, false, flags);
            break;
    };
    Q_ASSERT(m_trans);

//     connect(m_trans.data(), &PackageKit::Transaction::statusChanged, this, [this]() { qDebug() << "state..." << m_trans->status(); });
    connect(m_trans.data(), &PackageKit::Transaction::package, this, &PKTransaction::packageResolved);
    connect(m_trans.data(), &PackageKit::Transaction::finished, this, &PKTransaction::cleanup);
    connect(m_trans.data(), &PackageKit::Transaction::errorCode, this, &PKTransaction::errorFound);
    connect(m_trans.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PKTransaction::mediaChange);
    connect(m_trans.data(), &PackageKit::Transaction::requireRestart, this, &PKTransaction::requireRestart);
    connect(m_trans.data(), &PackageKit::Transaction::itemProgress, this, &PKTransaction::progressChanged);
    connect(m_trans.data(), &PackageKit::Transaction::eulaRequired, this, &PKTransaction::eulaRequired);
    connect(m_trans.data(), &PackageKit::Transaction::allowCancelChanged, this, &PKTransaction::cancellableChanged);
    
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::progressChanged(const QString &id, PackageKit::Transaction::Status status, uint percentage)
{
    Q_UNUSED(percentage);
    PackageKitResource * res = qobject_cast<PackageKitResource*>(resource());
    if (!res->allPackageNames().contains(PackageKit::Daemon::packageName(id)))
        return;

    if (status == PackageKit::Transaction::StatusDownload)
        setStatus(Transaction::DownloadingStatus);
    else
        setStatus(Transaction::CommittingStatus);
}

void PKTransaction::cancellableChanged()
{
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::cancel()
{
    if (!m_trans) {
        const auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());
        backend->transactionCanceled(this);
    } else if (m_trans->allowCancel()) {
        m_trans->cancel();
    } else {
        qWarning() << "trying to cancel a non-cancellable transaction: " << resource()->name();
    }
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    Q_UNUSED(runtime)
    const bool cancel = exit == PackageKit::Transaction::ExitEulaRequired || exit == PackageKit::Transaction::ExitCancelled;
    const bool simulate = m_trans->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_trans, nullptr, this, nullptr);
    m_trans = nullptr;

    const auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());

    if (!cancel && simulate) {
        auto packagesToRemove = m_newPackageStates.value(PackageKit::Transaction::InfoRemoving);
        QMutableListIterator<QString> i(packagesToRemove);
        QSet<AbstractResource*> removedResources;
        while (i.hasNext()) {
            const auto pkgname = PackageKit::Daemon::packageName(i.next());
            removedResources.unite(backend->searchPackageName(pkgname).toSet());

            if (m_pkgnames.contains(pkgname)) {
                i.remove();
            }
        }
        removedResources.subtract(m_apps.toList().toSet());

        QString msg = PackageKitResource::joinPackages(packagesToRemove);
        if (!removedResources.isEmpty()) {
            QStringList removedResourcesStr;
            removedResourcesStr.reserve(removedResources.size());
            foreach(AbstractResource* res, removedResources)
                removedResourcesStr.append(res->name());
            msg += QLatin1Char('\n');
            msg += removedResourcesStr.join(QStringLiteral(", "));
        }

        if (!msg.isEmpty()) {
            Q_EMIT proceedRequest(PackageKitMessages::statusMessage(PackageKit::Transaction::StatusRemove), msg);
        } else {
            proceed();
        }
        return;
    }

    this->submitResolve();
    setStatus(Transaction::DoneStatus);
    if (cancel)
        backend->transactionCanceled(this);
    else
        backend->removeTransaction(this);
    backend->fetchUpdates();
}

void PKTransaction::proceed()
{
    trigger(PackageKit::Transaction::TransactionFlagOnlyTrusted);
}

void PKTransaction::packageResolved(PackageKit::Transaction::Info info, const QString& packageId)
{
    m_newPackageStates[info].append(packageId);
}

void PKTransaction::submitResolve()
{
    QStringList needResolving;
    const auto pkgids = m_newPackageStates.value(PackageKit::Transaction::InfoFinished);
    foreach(const auto pkgid, pkgids) {
        needResolving += PackageKit::Daemon::packageName(pkgid);
    }
    const auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());
    backend->resolvePackages(needResolving);
}

PackageKit::Transaction* PKTransaction::transaction()
{
    return m_trans;
}

void PKTransaction::eulaRequired(const QString& eulaID, const QString& packageID, const QString& vendor, const QString& licenseAgreement)
{
    int ret = QMessageBox::question(nullptr, i18n("Accept EULA"), i18n("The package %1 and its vendor %2 require that you accept their license:\n %3",
                                                 PackageKit::Daemon::packageName(packageID), vendor, licenseAgreement));
    if (ret == QMessageBox::Yes) {
        PackageKit::Transaction* t = PackageKit::Daemon::acceptEula(eulaID);
        connect(t, &PackageKit::Transaction::finished, this, &PKTransaction::start);
    } else {
        cleanup(PackageKit::Transaction::ExitCancelled, 0);
    }
}

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement)
        return;
    qWarning() << "PackageKit error:" << err << PackageKitMessages::errorMessage(err) << error;
    QMessageBox::critical(nullptr, i18n("PackageKit Error"), PackageKitMessages::errorMessage(err));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text)
{
    Q_UNUSED(media)
    QMessageBox::information(nullptr, i18n("PackageKit media change"), i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

void PKTransaction::requireRestart(PackageKit::Transaction::Restart restart, const QString& pkgid)
{
    QMessageBox::information(nullptr, i18n("PackageKit restart required"), PackageKitMessages::restartMessage(restart, pkgid));
}
