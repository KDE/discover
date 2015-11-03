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

PKTransaction::PKTransaction(AbstractResource* app, Transaction::Role role)
    : Transaction(app, app, role),
      m_trans(nullptr)
{
}

void PKTransaction::start()
{
    if (m_trans)
        m_trans->deleteLater();

    switch (role()) {
        case Transaction::InstallRole:
            m_trans = PackageKit::Daemon::installPackage(qobject_cast<PackageKitResource*>(resource())->availablePackageId());
            break;
        case Transaction::RemoveRole:
            //see bug #315063
            m_trans = PackageKit::Daemon::removePackage(qobject_cast<PackageKitResource*>(resource())->installedPackageId(), true /*allowDeps*/);
            break;
        case Transaction::ChangeAddonsRole:
            qWarning() << "addons unsupported in PackageKit backend";
            break;
    };
    Q_ASSERT(m_trans);

    connect(m_trans, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(cleanup(PackageKit::Transaction::Exit,uint)));
    connect(m_trans, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(errorFound(PackageKit::Transaction::Error,QString)));
    connect(m_trans, SIGNAL(mediaChangeRequired(PackageKit::Transaction::MediaType,QString,QString)),
            SLOT(mediaChange(PackageKit::Transaction::MediaType,QString,QString)));
    connect(m_trans, SIGNAL(requireRestart(PackageKit::Transaction::Restart,QString)),
            SLOT(requireRestart(PackageKit::Transaction::Restart,QString)));
    connect(m_trans, SIGNAL(itemProgress(QString, PackageKit::Transaction::Status, uint)), SLOT(progressChanged(QString, PackageKit::Transaction::Status, uint)));
    connect(m_trans, SIGNAL(eulaRequired(QString, QString, QString, QString)), SLOT(eulaRequired(QString, QString, QString, QString)));
    connect(m_trans, SIGNAL(allowCancelChanged()), SLOT(cancellableChanged()));
    
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::progressChanged(const QString &id, PackageKit::Transaction::Status status, uint percentage)
{
    Q_UNUSED(percentage);
    PackageKitResource * res = qobject_cast<PackageKitResource*>(resource());
    if (id != res->availablePackageId() || id != res->installedPackageId())
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
    m_trans->cancel();
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    Q_UNUSED(runtime)
    if (exit == PackageKit::Transaction::ExitEulaRequired)
        return;

    if (exit == PackageKit::Transaction::ExitCancelled) {
        deleteLater();
    } else {
        disconnect(m_trans, nullptr, this, nullptr);
        m_trans = nullptr;
    }
    PackageKit::Transaction* t = PackageKit::Daemon::resolve(resource()->packageName(), PackageKit::Transaction::FilterArch | PackageKit::Transaction::FilterLast);
    connect(t, &PackageKit::Transaction::package, t, [t](PackageKit::Transaction::Info info, const QString& packageId) {
        QMap<PackageKit::Transaction::Info, QStringList> packages = t->property("packages").value<QMap<PackageKit::Transaction::Info, QStringList>>();
        packages[info].append(packageId);
        t->setProperty("packages", qVariantFromValue(packages));
    });

    connect(t, &PackageKit::Transaction::finished, t, [t, this](PackageKit::Transaction::Exit status, uint runtime){
        QMap<PackageKit::Transaction::Info, QStringList> packages = t->property("packages").value<QMap<PackageKit::Transaction::Info, QStringList>>();
        qobject_cast<PackageKitResource*>(resource())->setPackages(packages);
        setStatus(Transaction::DoneStatus);
        qobject_cast<PackageKitBackend*>(resource()->backend())->removeTransaction(this);
    });
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
        connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(start()));
    } else {
        cleanup(PackageKit::Transaction::ExitCancelled, 0);
    }
}

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    Q_UNUSED(error);
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement)
        return;
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
