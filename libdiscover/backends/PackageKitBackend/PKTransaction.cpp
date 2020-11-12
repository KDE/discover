/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PKTransaction.h"
#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include "PackageKitMessages.h"
#include "utils.h"
#include "LocalFilePKResource.h"
#include "libdiscover_backend_debug.h"
#include <resources/AbstractResource.h>
#include <QDebug>
#include <QTimer>
#include <KLocalizedString>
#include <PackageKit/Daemon>
#include <functional>

PKTransaction::PKTransaction(const QVector<AbstractResource*>& apps, Transaction::Role role)
    : Transaction(apps.first(), apps.first(), role)
    , m_apps(apps)
{
    Q_ASSERT(!apps.contains(nullptr));
    foreach(auto r, apps) {
        PackageKitResource* res = qobject_cast<PackageKitResource*>(r);
        m_pkgnames.unite(kToSet(res->allPackageNames()));
    }

    QTimer::singleShot(0, this, &PKTransaction::start);
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

    if (m_apps.size() == 1 && qobject_cast<LocalFilePKResource*>(m_apps.at(0))) {
        auto app = qobject_cast<LocalFilePKResource*>(m_apps.at(0));
        m_trans = PackageKit::Daemon::installFile(QUrl(app->packageName()).toLocalFile(), flags);
        connect(m_trans.data(), &PackageKit::Transaction::finished, this, [this, app](PackageKit::Transaction::Exit status) {
			const bool simulate = m_trans->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;
            if (!simulate && status == PackageKit::Transaction::ExitSuccess) {
                app->markInstalled();
            }
        });
    } else switch (role()) {
        case Transaction::ChangeAddonsRole:
        case Transaction::InstallRole: {
            const QStringList ids = packageIds(m_apps, [](PackageKitResource* r){return r->availablePackageId(); });
            if (ids.isEmpty()) {
                //FIXME this state shouldn't exist
                qWarning() << "Installing no packages found!";
                for(auto app : m_apps) {
                    qCDebug(LIBDISCOVER_BACKEND_LOG) << "app" << app << app->state() << static_cast<PackageKitResource*>(app)->packages();
                }

                setStatus(Transaction::DoneWithErrorStatus);
                return;
            }
            m_trans = PackageKit::Daemon::installPackages(ids, flags);
        }   break;
        case Transaction::RemoveRole:
            //see bug #315063
            m_trans = PackageKit::Daemon::removePackages(packageIds(m_apps, [](PackageKitResource* r){return r->installedPackageId(); }), true /*allowDeps*/, false, flags);
            break;
    };
    Q_ASSERT(m_trans);

//     connect(m_trans.data(), &PackageKit::Transaction::statusChanged, this, [this]() { qCDebug(LIBDISCOVER_BACKEND_LOG) << "state..." << m_trans->status(); });
    connect(m_trans.data(), &PackageKit::Transaction::package, this, &PKTransaction::packageResolved);
    connect(m_trans.data(), &PackageKit::Transaction::finished, this, &PKTransaction::cleanup);
    connect(m_trans.data(), &PackageKit::Transaction::errorCode, this, &PKTransaction::errorFound);
    connect(m_trans.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PKTransaction::mediaChange);
    connect(m_trans.data(), &PackageKit::Transaction::requireRestart, this, &PKTransaction::requireRestart);
    connect(m_trans.data(), &PackageKit::Transaction::repoSignatureRequired, this, &PKTransaction::repoSignatureRequired);
    connect(m_trans.data(), &PackageKit::Transaction::percentageChanged, this, &PKTransaction::progressChanged);
    connect(m_trans.data(), &PackageKit::Transaction::statusChanged, this, &PKTransaction::statusChanged);
    connect(m_trans.data(), &PackageKit::Transaction::eulaRequired, this, &PKTransaction::eulaRequired);
    connect(m_trans.data(), &PackageKit::Transaction::allowCancelChanged, this, &PKTransaction::cancellableChanged);
    connect(m_trans.data(), &PackageKit::Transaction::speedChanged, this, [this]() {
        setDownloadSpeed(m_trans->speed());
    });
    
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::statusChanged()
{
    setStatus(m_trans->status() == PackageKit::Transaction::StatusDownload ? Transaction::DownloadingStatus : Transaction::CommittingStatus);
    progressChanged();
}

int percentageWithStatus(PackageKit::Transaction::Status status, uint percentage);

void PKTransaction::progressChanged()
{
    auto percent = m_trans->percentage();
    if (percent == 101) {
        qWarning() << "percentage cannot be calculated";
        percent = 50;
    }

    const auto processedPercentage = percentageWithStatus(m_trans->status(), qBound<int>(0, percent, 100));
    if (processedPercentage >= 0)
        setProgress(processedPercentage);
}

void PKTransaction::cancellableChanged()
{
    setCancellable(m_trans->allowCancel());
}

void PKTransaction::cancel()
{
    if (!m_trans) {
        setStatus(CancelledStatus);
    } else if (m_trans->allowCancel()) {
        m_trans->cancel();
    } else {
        qWarning() << "trying to cancel a non-cancellable transaction: " << resource()->name();
    }
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    Q_UNUSED(runtime)
    const bool cancel = !m_proceedFunctions.isEmpty() || exit == PackageKit::Transaction::ExitCancelled;
    const bool failed = exit == PackageKit::Transaction::ExitFailed || exit == PackageKit::Transaction::ExitUnknown;
    const bool simulate = m_trans->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_trans, nullptr, this, nullptr);
    m_trans = nullptr;

    const auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());

    if (!cancel && !failed && simulate) {
        auto packagesToRemove = m_newPackageStates.value(PackageKit::Transaction::InfoRemoving);
        QMutableListIterator<QString> i(packagesToRemove);
        QSet<AbstractResource*> removedResources;
        while (i.hasNext()) {
            const auto pkgname = PackageKit::Daemon::packageName(i.next());
            removedResources.unite(backend->resourcesByPackageName(pkgname));

            if (m_pkgnames.contains(pkgname)) {
                i.remove();
            }
        }
        removedResources.subtract(kToSet(m_apps));

        if (!packagesToRemove.isEmpty() || !removedResources.isEmpty()) {
            QString msg = QLatin1String("<ul><li>") + PackageKitResource::joinPackages(packagesToRemove, QLatin1String("</li><li>"), {});
            if (!removedResources.isEmpty()) {
                const QStringList removedResourcesStr = kTransform<QStringList>(removedResources, [](AbstractResource* a) { return a->name(); });
                msg += QLatin1Char('\n');
                msg += removedResourcesStr.join(QLatin1String("</li><li>"));
            }
            msg += QStringLiteral("</li></ul>");

            Q_EMIT proceedRequest(i18n("Confirm package removal"), i18np("This action will also remove the following package:\n%2", "This action will also remove the following packages:\n%2", packagesToRemove.count(), msg));
        } else {
            proceed();
        }
        return;
    }

    this->submitResolve();
    if (failed)
        setStatus(Transaction::DoneWithErrorStatus);
    else if (cancel)
        setStatus(Transaction::CancelledStatus);
    else
        setStatus(Transaction::DoneStatus);
}

void PKTransaction::processProceedFunction()
{
    auto t = m_proceedFunctions.takeFirst()();
    connect(t, &PackageKit::Transaction::finished, this, [this](PackageKit::Transaction::Exit status) {
        if (status != PackageKit::Transaction::Exit::ExitSuccess) {
            qWarning() << "transaction failed" << sender() << status;
            cancel();
            return;
        }

        if (!m_proceedFunctions.isEmpty()) {
            processProceedFunction();
        } else {
            start();
        }
    });
}

void PKTransaction::proceed()
{
    if (!m_proceedFunctions.isEmpty()) {
        processProceedFunction();
    } else {
        if (m_apps.size() == 1 && qobject_cast<LocalFilePKResource*>(m_apps.at(0))) {
            trigger(PackageKit::Transaction::TransactionFlagNone);
        } else {
            trigger(PackageKit::Transaction::TransactionFlagOnlyTrusted);
        }
    }
}

void PKTransaction::packageResolved(PackageKit::Transaction::Info info, const QString& packageId)
{
    m_newPackageStates[info].append(packageId);
}

void PKTransaction::submitResolve()
{
    const auto backend = qobject_cast<PackageKitBackend*>(resource()->backend());
    QStringList needResolving;
    for(auto it = m_newPackageStates.constBegin(), itEnd = m_newPackageStates.constEnd(); it != itEnd; ++it) {
        auto state = it.key();
        if (state == PackageKit::Transaction::InfoInstalling)
            state = PackageKit::Transaction::InfoInstalled;
        if (state == PackageKit::Transaction::InfoRemoving)
            state = PackageKit::Transaction::InfoAvailable;
        if (state != PackageKit::Transaction::InfoInstalled && state != PackageKit::Transaction::InfoAvailable)
            continue;

        foreach(const auto &pkgid, it.value()) {
            const auto resources = backend->resourcesByPackageName(PackageKit::Daemon::packageName(pkgid));
            for(auto res: resources) {
                auto r = qobject_cast<PackageKitResource*>(res);
                r->clearPackageIds();
                r->addPackageId(state, pkgid, true);
            }
        }
    }
}

PackageKit::Transaction* PKTransaction::transaction()
{
    return m_trans;
}

void PKTransaction::eulaRequired(const QString& eulaID, const QString& packageID, const QString& vendor, const QString& licenseAgreement)
{
    m_proceedFunctions << [eulaID](){
        return PackageKit::Daemon::acceptEula(eulaID);
    };

    Q_EMIT proceedRequest(i18n("Accept EULA"), i18n("The package %1 and its vendor %2 require that you accept their license:\n %3",
                                                 PackageKit::Daemon::packageName(packageID), vendor, licenseAgreement));
}

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement || err == PackageKit::Transaction::ErrorTransactionCancelled)
        return;
    qWarning() << "PackageKit error:" << err << PackageKitMessages::errorMessage(err) << error;
    Q_EMIT passiveMessage(PackageKitMessages::errorMessage(err));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

void PKTransaction::requireRestart(PackageKit::Transaction::Restart restart, const QString& pkgid)
{
    Q_EMIT passiveMessage(PackageKitMessages::restartMessage(restart, pkgid));
}

void PKTransaction::repoSignatureRequired(const QString& packageID, const QString& repoName, const QString& keyUrl,
                                          const QString& keyUserid, const QString& keyId, const QString& keyFingerprint,
                                          const QString& keyTimestamp, PackageKit::Transaction::SigType type)
{
    Q_EMIT proceedRequest(i18n("Missing signature for %1 in %2", packageID, repoName),
                          i18n("Do you trust the following key?\n\nUrl: %1\nUser: %2\nKey: %3\nFingerprint: %4\nTimestamp: %4\n",
                               keyUrl, keyUserid, keyFingerprint, keyTimestamp));

    m_proceedFunctions << [type, keyId, packageID](){
        return PackageKit::Daemon::installSignature(type, keyId, packageID);
    };
}
