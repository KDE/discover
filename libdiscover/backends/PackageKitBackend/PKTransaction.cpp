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

PKTransaction::PKTransaction(const QVector<AbstractResource*>& apps, Transaction::Role role)
    : Transaction(apps.first(), apps.first(), role)
    , m_apps(apps)
{
    Q_ASSERT(!apps.contains(nullptr));
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
    if (m_trans)
        m_trans->deleteLater();

    switch (role()) {
        case Transaction::ChangeAddonsRole:
        case Transaction::InstallRole:
            m_trans = PackageKit::Daemon::installPackages(packageIds(m_apps, [](PackageKitResource* r){return r->availablePackageId(); }));
            break;
        case Transaction::RemoveRole:
            //see bug #315063
            m_trans = PackageKit::Daemon::removePackages(packageIds(m_apps, [](PackageKitResource* r){return r->installedPackageId(); }), true /*allowDeps*/);
            break;
    };
    Q_ASSERT(m_trans);

    connect(m_trans, &PackageKit::Transaction::finished, this, &PKTransaction::cleanup);
    connect(m_trans, &PackageKit::Transaction::errorCode, this, &PKTransaction::errorFound);
    connect(m_trans, &PackageKit::Transaction::mediaChangeRequired, this, &PKTransaction::mediaChange);
    connect(m_trans, &PackageKit::Transaction::requireRestart, this, &PKTransaction::requireRestart);
    connect(m_trans, &PackageKit::Transaction::itemProgress, this, &PKTransaction::progressChanged);
    connect(m_trans, &PackageKit::Transaction::eulaRequired, this, &PKTransaction::eulaRequired);
    connect(m_trans, &PackageKit::Transaction::allowCancelChanged, this, &PKTransaction::cancellableChanged);
    
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
    if (m_trans->allowCancel()) {
        m_trans->cancel();
    } else {
        qWarning() << "trying to cancel a non-cancellable transaction: " << resource()->name();
    }
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    Q_UNUSED(runtime)
    bool cancel = false;
    if (exit == PackageKit::Transaction::ExitEulaRequired || exit == PackageKit::Transaction::ExitCancelled) {
        cancel = true;
    }

    disconnect(m_trans, nullptr, this, nullptr);
    m_trans = nullptr;

    PackageKit::Transaction* t = PackageKit::Daemon::resolve(resource()->packageName(), PackageKit::Transaction::FilterArch);
    connect(t, &PackageKit::Transaction::package, this, &PKTransaction::packageResolved);

    connect(t, &PackageKit::Transaction::finished, t, [cancel, this](PackageKit::Transaction::Exit /*status*/, uint /*runtime*/) {
        this->submitResolve();
        auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());
        if (cancel) backend->transactionCanceled(this);
        else backend->removeTransaction(this);
        backend->fetchUpdates();
    });
}

void PKTransaction::packageResolved(PackageKit::Transaction::Info info, const QString& packageId)
{
    m_newPackageStates[info].append(packageId);
}

void PKTransaction::submitResolve()
{
    qobject_cast<PackageKitResource*>(resource())->setPackages(m_newPackageStates);
    setStatus(Transaction::DoneStatus);
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
    Q_UNUSED(error);
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement)
        return;
    qWarning() << "PackageKit error:" << err << PackageKitMessages::errorMessage(err);
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
