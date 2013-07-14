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
#include <resources/AbstractResource.h>
#include <Transaction/TransactionModel.h>
#include <KMessageBox>
#include <KLocalizedString>
#include <PackageKit/packagekit-qt2/Transaction>
#include <PackageKit/packagekit-qt2/Daemon>
#include <KDebug>

PKTransaction::PKTransaction(AbstractResource* app, Transaction::Role role, PackageKit::Transaction* pktrans)
    : Transaction(app, app, role)
    , m_trans(pktrans)
{
    m_trans->setParent(this);
    setCancellable(pktrans->allowCancel());
    connect(pktrans, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(cleanup(PackageKit::Transaction::Exit,uint)));
    connect(pktrans, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(errorFound(PackageKit::Transaction::Error,QString)));
    connect(pktrans, SIGNAL(mediaChangeRequired(PackageKit::Transaction::MediaType,QString,QString)),
            SLOT(mediaChange(PackageKit::Transaction::MediaType,QString,QString)));
    connect(pktrans, SIGNAL(requireRestart(PackageKit::Transaction::Restart,QString)),
            SLOT(requireRestard(PackageKit::Transaction::Restart,QString)));
    connect(pktrans, SIGNAL(itemProgress(QString, PackageKit::Transaction::Status, uint)), SLOT(progressChanged(QString, PackageKit::Transaction::Status, uint)));
    connect(pktrans, SIGNAL(eulaRequired(QString, QString, QString, QString)), SLOT(eulaRequired(QString, QString, QString, QString)));
}

void PKTransaction::progressChanged(const QString &id, PackageKit::Transaction::Status status, uint percentage)
{
    PackageKitResource * res = qobject_cast<PackageKitResource*>(resource());
    if (id != res->availablePackageId() ||
        id != res->installedPackageId())
        return;
    kDebug() << "Progress" << percentage << "state" << status;
    setProgress(percentage);
    if (status == PackageKit::Transaction::StatusDownload)
        setStatus(Transaction::DownloadingStatus);
    else
        setStatus(Transaction::CommittingStatus);
}

void PKTransaction::cancel()
{
    m_trans->cancel();
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    setStatus(Transaction::DoneStatus);
    if (exit == PackageKit::Transaction::ExitCancelled) {
        TransactionModel::global()->cancelTransaction(this);
        deleteLater();
    } else {
        qobject_cast<PackageKitBackend*>(resource()->backend())->removeTransaction(this);
    }
    PackageKit::Transaction* t = new PackageKit::Transaction(resource());
    t->resolve(resource()->packageName(), PackageKit::Transaction::FilterArch | PackageKit::Transaction::FilterLast);
    connect(t, SIGNAL(package(PackageKit::Transaction::Info,QString,QString)), resource(), SLOT(addPackageId(PackageKit::Transaction::Info, QString,QString)));
    connect(t, SIGNAL(destroy()), t, SLOT(deleteLater()));
}

PackageKit::Transaction* PKTransaction::transaction()
{
    return m_trans;
}

void PKTransaction::eulaRequired(const QString& eulaID, const QString& packageID, const QString& vendor, const QString& licenseAgreement)
{
    int ret = KMessageBox::questionYesNo(0, i18n("The package %1 and its vendor %2 require that you accept their license:\n %3", 
                                                 PackageKit::Daemon::packageName(packageID), vendor, licenseAgreement),
                                            i18n("%1 requires user to accept its license!", PackageKit::Daemon::packageName(packageID)));
    if (ret == KMessageBox::Yes) {
        m_trans->acceptEula(eulaID);
    } else {
        m_trans->cancel();
    }
}

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    KMessageBox::error(0, error, PackageKitBackend::errorMessage(err));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text)
{
    KMessageBox::information(0, text, i18n("Media Change of type '%1' is requested.", type));
}

void PKTransaction::requireRestard(PackageKit::Transaction::Restart restart, const QString& p)
{
    QString message;
    switch (restart) {
        case PackageKit::Transaction::RestartApplication:
            message = i18n("'%1' was changed and suggests to be restarted.", PackageKit::Daemon::packageName(p));
            break;
        case PackageKit::Transaction::RestartSession:
            message = i18n("A change by '%1' suggests your session to be restarted.", PackageKit::Daemon::packageName(p));
            break;
        case PackageKit::Transaction::RestartSecuritySession:
            message = i18n("'%1' was updated for security reasons, a restart of the session is recommended.", PackageKit::Daemon::packageName(p));
            break;
        case PackageKit::Transaction::RestartSecuritySystem:
            message = i18n("'%1' was updated for security reasons, a restart of the system is recommended.", PackageKit::Daemon::packageName(p));
            break;
        case PackageKit::Transaction::RestartSystem:
        case PackageKit::Transaction::RestartUnknown:
        case PackageKit::Transaction::RestartNone:
        default:
            message = i18n("A change by '%1' suggests your system to be rebooted.", PackageKit::Daemon::packageName(p));
            break;
    };
    KMessageBox::information(0, message);
}
