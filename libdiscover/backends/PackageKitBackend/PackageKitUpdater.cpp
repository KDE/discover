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
#include "PackageKitUpdater.h"
#include "PackageKitMessages.h"

#include <PackageKit/Daemon>
#include <QDebug>
#include <QAction>
#include <QSet>

#include <KLocalizedString>

static int percentageWithStatus(PackageKit::Transaction::Status status, uint percentage)
{
    if (status != PackageKit::Transaction::StatusUnknown) {
        static QVector<PackageKit::Transaction::Status> statuses = {PackageKit::Transaction::Status::StatusDownload, PackageKit::Transaction::Status::StatusUpdate};
        const auto idx = statuses.indexOf(status);
        if (idx < 0) {
            qDebug() << "Status not present" << status << percentage;
            return -1;
        }
        percentage = (idx * 100 + percentage) / statuses.count();
    }
    qDebug() << "reporing progress:" << status << percentage;
    return percentage;
}

PackageKitUpdater::PackageKitUpdater(PackageKitBackend * parent)
  : AbstractBackendUpdater(parent),
    m_transaction(nullptr),
    m_backend(parent),
    m_isCancelable(false),
    m_isProgressing(false),
    m_status(PackageKit::Transaction::Status::StatusUnknown),
    m_percentage(0),
    m_lastUpdate()
{
    fetchLastUpdateTime();
}

PackageKitUpdater::~PackageKitUpdater()
{
    delete m_transaction;
}

void PackageKitUpdater::prepare()
{
    Q_ASSERT(!m_transaction);
    m_toUpgrade = m_backend->upgradeablePackages();
    m_allUpgradeable = m_toUpgrade;
}

void PackageKitUpdater::setupTransaction(PackageKit::Transaction::TransactionFlags flags)
{
    m_packagesRemoved.clear();
    m_transaction = PackageKit::Daemon::updatePackages(involvedPackages(m_toUpgrade).toList(), flags);
    m_isCancelable = m_transaction->allowCancel();
    m_status = PackageKit::Transaction::Status::StatusUnknown;

    connect(m_transaction.data(), &PackageKit::Transaction::finished, this, &PackageKitUpdater::finished);
    connect(m_transaction.data(), &PackageKit::Transaction::package, this, &PackageKitUpdater::packageResolved);
    connect(m_transaction.data(), &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
    connect(m_transaction.data(), &PackageKit::Transaction::message, this, &PackageKitUpdater::printMessage);
    connect(m_transaction.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PackageKitUpdater::mediaChange);
    connect(m_transaction.data(), &PackageKit::Transaction::requireRestart, this, &PackageKitUpdater::requireRestart);
    connect(m_transaction.data(), &PackageKit::Transaction::eulaRequired, this, &PackageKitUpdater::eulaRequired);
    connect(m_transaction.data(), &PackageKit::Transaction::statusChanged, this, &PackageKitUpdater::statusChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::allowCancelChanged, this, &PackageKitUpdater::cancellableChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::percentageChanged, this, &PackageKitUpdater::percentageChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::itemProgress, this, &PackageKitUpdater::itemProgress);
}

QSet<AbstractResource*> PackageKitUpdater::packagesForPackageId(const QSet<QString>& pkgids) const
{
    QSet<QString> packages;
    packages.reserve(pkgids.size());
    foreach(const QString& pkgid, pkgids) {
        packages += PackageKit::Daemon::packageName(pkgid);
    }

    QSet<AbstractResource*> ret;
    foreach (AbstractResource * res, m_allUpgradeable) {
        PackageKitResource* pres = qobject_cast<PackageKitResource*>(res);
        if (packages.contains(pres->allPackageNames().toSet())) {
            ret.insert(res);
        }
    }

    return ret;
}

QSet<QString> PackageKitUpdater::involvedPackages(const QSet<AbstractResource*>& packages) const
{
    QSet<QString> packageIds;
    packageIds.reserve(packages.size());
    foreach (AbstractResource * res, packages) {
        PackageKitResource * app = qobject_cast<PackageKitResource*>(res);
        QString pkgid = m_backend->upgradeablePackageId(app);
        packageIds.insert(pkgid);
    }
    return packageIds;
}

void PackageKitUpdater::proceed()
{
    if (!m_requiredEula.isEmpty()) {
        PackageKit::Transaction* t = PackageKit::Daemon::acceptEula(m_requiredEula.takeFirst());
        connect(t, &PackageKit::Transaction::finished, this, &PackageKitUpdater::start);
        return;
    }
    setupTransaction(PackageKit::Transaction::TransactionFlagOnlyTrusted);
}

void PackageKitUpdater::start()
{
    Q_ASSERT(!isProgressing());

    setupTransaction(PackageKit::Transaction::TransactionFlagSimulate);
    setProgressing(true);
}

void PackageKitUpdater::finished(PackageKit::Transaction::Exit exit, uint /*time*/)
{
//     qDebug() << "update finished!" << exit << time;
    if (exit == PackageKit::Transaction::ExitEulaRequired)
        return;
    const bool cancel = exit == PackageKit::Transaction::ExitCancelled;
    const bool simulate = m_transaction->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_transaction, nullptr, this, nullptr);
    m_transaction = nullptr;

    if (!cancel && simulate) {
        if (!m_packagesRemoved.isEmpty())
            Q_EMIT proceedRequest(i18n("Packages to remove"), i18n("The following packages will be removed by the update:\n%1", PackageKitResource::joinPackages(m_packagesRemoved)));
        else {
            Q_ASSERT(m_requiredEula.isEmpty());
            proceed();
        }
        return;
    }

    setProgressing(false);
    m_backend->refreshDatabase();
    fetchLastUpdateTime();
}

void PackageKitUpdater::cancellableChanged()
{
    if (m_isCancelable != m_transaction->allowCancel()) {
        m_isCancelable = m_transaction->allowCancel();
        emit cancelableChanged(m_isCancelable);
    }
}

void PackageKitUpdater::percentageChanged()
{
    const auto actualPercentage = percentageWithStatus(m_transaction->status(), m_transaction->percentage());
    if (actualPercentage >= 0 && m_percentage != actualPercentage) {
        m_percentage = actualPercentage;
        emit progressChanged(m_percentage);
    }
}

void PackageKitUpdater::statusChanged()
{
    if (m_status != m_transaction->status()) {
        m_status = m_transaction->status();
    }
}

bool PackageKitUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

qreal PackageKitUpdater::progress() const
{
    return m_percentage;
}

void PackageKitUpdater::removeResources(const QList<AbstractResource*>& apps)
{
    QSet<QString> pkgs = involvedPackages(apps.toSet());
    m_toUpgrade.subtract(packagesForPackageId(pkgs));
}

void PackageKitUpdater::addResources(const QList<AbstractResource*>& apps)
{
    QSet<QString> pkgs = involvedPackages(apps.toSet());
    m_toUpgrade.unite(packagesForPackageId(pkgs));
}

QList<AbstractResource*> PackageKitUpdater::toUpdate() const
{
    return m_toUpgrade.toList();
}

bool PackageKitUpdater::isMarked(AbstractResource* res) const
{
    return m_toUpgrade.contains(res);
}

QDateTime PackageKitUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

bool PackageKitUpdater::isCancelable() const
{
    return m_isCancelable;
}

bool PackageKitUpdater::isProgressing() const
{
    return m_isProgressing;
}

void PackageKitUpdater::cancel()
{
    if (m_transaction)
        m_transaction->cancel();
    else
        setProgressing(false);
}

void PackageKitUpdater::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement)
        return;
    Q_EMIT passiveMessage(QStringLiteral("%1\n%2").arg(PackageKitMessages::errorMessage(err), error));
    qWarning() << "Error happened" << err << error;
}

void PackageKitUpdater::printMessage(PackageKit::Transaction::Message type, const QString &message)
{
    qDebug() << "message" << type << message;
}

void PackageKitUpdater::mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

void PackageKitUpdater::requireRestart(PackageKit::Transaction::Restart restart, const QString& pkgid)
{
    Q_EMIT passiveMessage(PackageKitMessages::restartMessage(restart, pkgid));
}

void PackageKitUpdater::eulaRequired(const QString& eulaID, const QString& packageID, const QString& vendor, const QString& licenseAgreement)
{
    m_requiredEula += eulaID;
    Q_EMIT proceedRequest(i18n("Accept EULA"), i18n("The package %1 and its vendor %2 require that you accept their license:\n %3",
                                                 PackageKit::Daemon::packageName(packageID), vendor, licenseAgreement));
}

void PackageKitUpdater::setProgressing(bool progressing)
{
    if (m_isProgressing != progressing) {
        m_isProgressing = progressing;
        emit progressingChanged(m_isProgressing);
    }
}

void PackageKitUpdater::fetchLastUpdateTime()
{
    QDBusPendingReply<uint> transaction = PackageKit::Daemon::global()->getTimeSinceAction(PackageKit::Transaction::RoleGetUpdates);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(transaction, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PackageKitUpdater::lastUpdateTimeReceived);
}

void PackageKitUpdater::lastUpdateTimeReceived(QDBusPendingCallWatcher* w)
{
    QDBusPendingReply<uint> reply = w->reply();
    if (reply.isError()) {
        qWarning() << "Error when fetching the last update time" << reply.error();
    } else {
        m_lastUpdate = QDateTime::currentDateTime().addSecs(-int(reply.value()));
    }
    w->deleteLater();
}

void PackageKitUpdater::itemProgress(const QString& itemID, PackageKit::Transaction::Status status, uint percentage)
{
    auto res = packagesForPackageId({itemID});

    const auto actualPercentage = percentageWithStatus(status, percentage);
    if (actualPercentage<0)
        return;

    foreach(auto r, res) {
        Q_EMIT resourceProgressed(r, actualPercentage);
    }
}

void PackageKitUpdater::fetchChangelog() const
{
    QStringList pkgids;
    foreach(AbstractResource* res, m_allUpgradeable) {
        pkgids += static_cast<PackageKitResource*>(res)->availablePackageId();
    }
    Q_ASSERT(!pkgids.isEmpty());

    PackageKit::Transaction* t = PackageKit::Daemon::getUpdatesDetails(pkgids);
    connect(t, &PackageKit::Transaction::updateDetail, this, &PackageKitUpdater::updateDetail);
    connect(t, &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
}

void PackageKitUpdater::updateDetail(const QString& packageID, const QStringList& updates, const QStringList& obsoletes, const QStringList& vendorUrls,
                                      const QStringList& bugzillaUrls, const QStringList& cveUrls, PackageKit::Transaction::Restart restart, const QString& updateText,
                                      const QString& changelog, PackageKit::Transaction::UpdateState state, const QDateTime& issued, const QDateTime& updated)
{
    auto res = packagesForPackageId({packageID});
    foreach(auto r, res) {
        static_cast<PackageKitResource*>(r)->updateDetail(packageID, updates, obsoletes, vendorUrls, bugzillaUrls,
                                                          cveUrls, restart, updateText, changelog, state, issued, updated);
    }
}

void PackageKitUpdater::packageResolved(PackageKit::Transaction::Info info, const QString& packageId)
{
    if (info == PackageKit::Transaction::InfoRemoving)
        m_packagesRemoved << packageId;
}
