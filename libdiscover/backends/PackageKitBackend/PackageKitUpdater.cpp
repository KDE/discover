/*
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "PackageKitUpdater.h"
#include "PackageKitMessages.h"

#include <PackageKit/Daemon>
#include <PackageKit/Offline>
#include <QCryptographicHash>
#include <QDebug>
#include <QSet>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "libdiscover_backend_debug.h"
#include "utils.h"

int percentageWithStatus(PackageKit::Transaction::Status status, uint percentage)
{
    const auto was = percentage;
    if (status != PackageKit::Transaction::StatusUnknown) {
        static const QMap<PackageKit::Transaction::Status, int> statuses = {
            {PackageKit::Transaction::Status::StatusDownload, 0},
            {PackageKit::Transaction::Status::StatusInstall, 1},
            {PackageKit::Transaction::Status::StatusRemove, 1},
            {PackageKit::Transaction::Status::StatusLoadingCache, 1},
            {PackageKit::Transaction::Status::StatusUpdate, 1},
        };
        const auto idx = statuses.value(status, -1);
        if (idx < 0) {
            qCDebug(LIBDISCOVER_BACKEND_LOG) << "Status not present" << status << "among" << statuses.keys() << percentage;
            return -1;
        }
        percentage = (idx * 100 + percentage) / 2 /*the maximum in statuses*/;
    }
    qCDebug(LIBDISCOVER_BACKEND_LOG) << "reporting progress with status:" << status << percentage << was;
    return percentage;
}

static void kRemoveDuplicates(QJsonArray &input, std::function<QString(const QJsonValueRef &)> fetchKey)
{
    QSet<QString> ret;
    for (auto it = input.begin(); it != input.end();) {
        const auto key = fetchKey(*it);
        if (!ret.contains(key)) {
            ret << key;
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

class SystemUpgrade : public AbstractResource
{
public:
    SystemUpgrade(PackageKitBackend *backend)
        : AbstractResource(backend)
        , m_backend(backend)
    {
        connect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, [this](AbstractResource *res) {
            m_resources.remove(res);
        });
    }

    QString packageName() const override
    {
        return QStringLiteral("discover-offline-upgrade");
    }
    QString name() const override
    {
        return i18n("System upgrade");
    }
    QString comment() override
    {
        return upgradeText();
    }
    QVariant icon() const override
    {
        return QStringLiteral("system-upgrade");
    }
    bool canExecute() const override
    {
        return false;
    }
    void invokeApplication() const override
    {
    }
    State state() override
    {
        return Upgradeable;
    }
    QStringList categories() override
    {
        return {};
    }
    AbstractResource::Type type() const override
    {
        return Technical;
    }
    bool isRemovable() const override
    {
        return false;
    }

    QVector<PackageKitResource *> withoutDuplicates() const
    {
        QVector<PackageKitResource *> ret;
        QSet<QString> donePkgs;
        for (auto res : qAsConst(m_resources)) {
            PackageKitResource *app = qobject_cast<PackageKitResource *>(res);
            QString pkgid = m_backend->upgradeablePackageId(app);
            if (!donePkgs.contains(pkgid)) {
                donePkgs.insert(pkgid);
                ret += app;
            }
        }
        return ret;
    }

    int size() override
    {
        int ret = 0;
        const auto resources = withoutDuplicates();
        for (auto res : resources) {
            ret += res->size();
        }
        return ret;
    }
    QJsonArray licenses() override
    {
        QJsonArray ret;
        for (auto res : qAsConst(m_resources)) {
            ret += res->licenses();
        }
        kRemoveDuplicates(ret, [](const QJsonValueRef &val) -> QString {
            return val.toObject()[QLatin1String("name")].toString();
        });
        return ret;
    }
    QString section() override
    {
        return {};
    }
    QString longDescription() override
    {
        return {};
    }
    QString origin() const override
    {
        return {};
    }
    QString author() const override
    {
        return {};
    }
    QList<PackageState> addonsInformation() override
    {
        return {};
    }
    QString upgradeText() const override
    {
        return i18np("1 package to upgrade", "%1 packages to upgrade", withoutDuplicates().count());
    }
    void fetchChangelog() override
    {
        QStringList changes;
        const auto resources = withoutDuplicates();
        for (auto res : resources) {
            const auto versions = res->upgradeText();
            const auto idx = versions.indexOf(u'\u009C');
            changes += QStringLiteral("<li>") + res->packageName() + QStringLiteral(": ") + versions.leftRef(idx) + QStringLiteral("</li>\n");
        }
        changes.sort();
        Q_EMIT changelogFetched(QStringLiteral("<ul>") + changes.join(QString()) + QStringLiteral("</ul>\n"));
    }

    QString installedVersion() const override
    {
        return i18n("Present");
    }
    QString availableVersion() const override
    {
        return i18n("Future");
    }
    QString sourceIcon() const override
    {
        return QStringLiteral("package-x-generic");
    }
    QDate releaseDate() const override
    {
        return {};
    }

    QSet<AbstractResource *> resources() const
    {
        return m_resources;
    }

    QSet<QString> allPackageNames() const
    {
        QSet<QString> ret;
        for (auto res : qAsConst(m_resources)) {
            ret += kToSet(qobject_cast<PackageKitResource *>(res)->allPackageNames());
        }
        return ret;
    }

    void refreshResource()
    {
        Q_EMIT m_backend->resourcesChanged(this, {"size", "license"});
    }

    void setCandidates(const QSet<AbstractResource *> &candidates)
    {
        for (auto res : (m_resources - candidates)) {
            disconnect(res, &AbstractResource::sizeChanged, this, &SystemUpgrade::refreshResource);
        }

        const auto newCandidates = (candidates - m_resources);
        m_resources = candidates;
        for (auto res : newCandidates) {
            connect(res, &AbstractResource::sizeChanged, this, &SystemUpgrade::refreshResource);
        }
    }

private:
    QSet<AbstractResource *> m_resources;
    PackageKitBackend *const m_backend;
};

PackageKitUpdater::PackageKitUpdater(PackageKitBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_transaction(nullptr)
    , m_backend(parent)
    , m_isCancelable(false)
    , m_isProgressing(false)
    , m_percentage(0)
    , m_lastUpdate()
    , m_upgrade(new SystemUpgrade(m_backend))
{
    fetchLastUpdateTime();
}

PackageKitUpdater::~PackageKitUpdater()
{
}

void PackageKitUpdater::prepare()
{
    if (PackageKit::Daemon::global()->offline()->updateTriggered()) {
        m_toUpgrade.clear();
        m_allUpgradeable.clear();
        enableNeedsReboot();
        return;
    }

    Q_ASSERT(!m_transaction);
    const auto candidates = m_backend->upgradeablePackages();
    if (useOfflineUpdates() && !candidates.isEmpty()) {
        m_upgrade->setCandidates(candidates);

        m_toUpgrade = {m_upgrade};
    } else {
        m_toUpgrade = candidates;
    }
    m_allUpgradeable = m_toUpgrade;
}

void PackageKitUpdater::setupTransaction(PackageKit::Transaction::TransactionFlags flags)
{
    m_packagesModified.clear();
    auto pkgs = involvedPackages(m_toUpgrade).values();
    pkgs.sort();
    m_transaction = PackageKit::Daemon::updatePackages(pkgs, flags);
    m_isCancelable = m_transaction->allowCancel();
    cancellableChanged();

    connect(m_transaction.data(), &PackageKit::Transaction::finished, this, &PackageKitUpdater::finished);
    connect(m_transaction.data(), &PackageKit::Transaction::package, this, &PackageKitUpdater::packageResolved);
    connect(m_transaction.data(), &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
    connect(m_transaction.data(), &PackageKit::Transaction::mediaChangeRequired, this, &PackageKitUpdater::mediaChange);
    connect(m_transaction.data(), &PackageKit::Transaction::eulaRequired, this, &PackageKitUpdater::eulaRequired);
    connect(m_transaction.data(), &PackageKit::Transaction::repoSignatureRequired, this, &PackageKitUpdater::repoSignatureRequired);
    connect(m_transaction.data(), &PackageKit::Transaction::allowCancelChanged, this, &PackageKitUpdater::cancellableChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::percentageChanged, this, &PackageKitUpdater::percentageChanged);
    connect(m_transaction.data(), &PackageKit::Transaction::itemProgress, this, &PackageKitUpdater::itemProgress);
    connect(m_transaction.data(), &PackageKit::Transaction::speedChanged, this, [this] {
        Q_EMIT downloadSpeedChanged(downloadSpeed());
    });
    if (m_toUpgrade.contains(m_upgrade)) {
        connect(m_transaction, &PackageKit::Transaction::percentageChanged, this, [this] {
            if (m_transaction->status() == PackageKit::Transaction::StatusDownload) {
                Q_EMIT resourceProgressed(m_upgrade, m_transaction->percentage(), Downloading);
            }
        });
    }
}

QSet<AbstractResource *> PackageKitUpdater::packagesForPackageId(const QSet<QString> &pkgids) const
{
    const auto packages = kTransform<QSet<QString>>(pkgids, [](const QString &pkgid) {
        return PackageKit::Daemon::packageName(pkgid);
    });

    QSet<AbstractResource *> ret;
    foreach (AbstractResource *res, m_allUpgradeable) {
        if (auto upgrade = dynamic_cast<SystemUpgrade *>(res)) {
            if (packages.contains(upgrade->allPackageNames())) {
                ret += upgrade;
            }
            continue;
        }

        PackageKitResource *pres = qobject_cast<PackageKitResource *>(res);
        if (packages.contains(kToSet(pres->allPackageNames()))) {
            ret.insert(res);
        }
    }

    return ret;
}

QSet<QString> PackageKitUpdater::involvedPackages(const QSet<AbstractResource *> &packages) const
{
    QSet<QString> packageIds;
    packageIds.reserve(packages.size());
    foreach (AbstractResource *res, packages) {
        if (SystemUpgrade *upgrade = dynamic_cast<SystemUpgrade *>(res)) {
            packageIds = involvedPackages(upgrade->resources());
            continue;
        }

        PackageKitResource *app = qobject_cast<PackageKitResource *>(res);
        const QString pkgid = m_backend->upgradeablePackageId(app);
        if (pkgid.isEmpty()) {
            qWarning() << "no upgradeablePackageId for" << app;
            continue;
        }

        packageIds.insert(pkgid);
    }
    return packageIds;
}

void PackageKitUpdater::processProceedFunction()
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

void PackageKitUpdater::proceed()
{
    if (!m_proceedFunctions.isEmpty())
        processProceedFunction();
    else if (useOfflineUpdates())
        setupTransaction(PackageKit::Transaction::TransactionFlagOnlyTrusted | PackageKit::Transaction::TransactionFlagOnlyDownload);
    else
        setupTransaction(PackageKit::Transaction::TransactionFlagOnlyTrusted);
}

bool PackageKitUpdater::useOfflineUpdates() const
{
    return m_useOfflineUpdates || qEnvironmentVariableIntValue("PK_OFFLINE_UPDATE");
}

void PackageKitUpdater::setOfflineUpdates(bool use)
{
    m_useOfflineUpdates = use;
}

void PackageKitUpdater::start()
{
    Q_ASSERT(!isProgressing());

    setupTransaction(PackageKit::Transaction::TransactionFlagSimulate);
    setProgressing(true);
}

void PackageKitUpdater::finished(PackageKit::Transaction::Exit exit, uint /*time*/)
{
    // qCDebug(LIBDISCOVER_BACKEND_LOG) << "update finished!" << exit << time;
    if (!m_proceedFunctions.isEmpty())
        return;
    const bool cancel = exit == PackageKit::Transaction::ExitCancelled;
    const bool simulate = m_transaction->transactionFlags() & PackageKit::Transaction::TransactionFlagSimulate;

    disconnect(m_transaction, nullptr, this, nullptr);
    m_transaction = nullptr;

    if (!cancel && simulate) {
        const auto toremove = m_packagesModified.value(PackageKit::Transaction::InfoRemoving);
        if (!toremove.isEmpty()) {
            const auto toinstall = QStringList() << m_packagesModified.value(PackageKit::Transaction::InfoInstalling)
                                                 << m_packagesModified.value(PackageKit::Transaction::InfoUpdating);
            Q_EMIT proceedRequest(i18n("Packages to remove"),
                                  i18n("The following packages will be removed by the update:<ul><li>%1</li></ul><br/>in order to install:<ul><li>%2</li></ul>",
                                       PackageKitResource::joinPackages(toremove, QStringLiteral("</li><li>"), {}),
                                       PackageKitResource::joinPackages(toinstall, QStringLiteral("</li><li>"), {})));
        } else {
            proceed();
        }
        return;
    }

    setProgressing(false);
    m_backend->fetchUpdates();
    fetchLastUpdateTime();

    if (useOfflineUpdates() && exit == PackageKit::Transaction::ExitSuccess) {
        PackageKit::Daemon::global()->offline()->trigger(PackageKit::Offline::ActionReboot);
    }
}

void PackageKitUpdater::cancellableChanged()
{
    if (m_isCancelable != m_transaction->allowCancel()) {
        m_isCancelable = m_transaction->allowCancel();
        Q_EMIT cancelableChanged(m_isCancelable);
    }
}

void PackageKitUpdater::percentageChanged()
{
    const auto actualPercentage = percentageWithStatus(m_transaction->status(), m_transaction->percentage());
    if (actualPercentage >= 0 && m_percentage != actualPercentage) {
        m_percentage = actualPercentage;
        Q_EMIT progressChanged(m_percentage);
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

void PackageKitUpdater::removeResources(const QList<AbstractResource *> &apps)
{
    const QSet<QString> pkgs = involvedPackages(kToSet(apps));
    m_toUpgrade.subtract(packagesForPackageId(pkgs));
}

void PackageKitUpdater::addResources(const QList<AbstractResource *> &apps)
{
    const QSet<QString> pkgs = involvedPackages(kToSet(apps));
    m_toUpgrade.unite(packagesForPackageId(pkgs));
}

QList<AbstractResource *> PackageKitUpdater::toUpdate() const
{
    return m_toUpgrade.values();
}

bool PackageKitUpdater::isMarked(AbstractResource *res) const
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

void PackageKitUpdater::errorFound(PackageKit::Transaction::Error err, const QString &error)
{
    if (err == PackageKit::Transaction::ErrorNoLicenseAgreement)
        return;
    QString finalMessage = xi18nc("@info",
                                  "%1:<nl/><nl/>%2<nl/><nl/>Please report this issue to the packagers of your distribution.",
                                  PackageKitMessages::errorMessage(err),
                                  error);
    Q_EMIT passiveMessage(finalMessage);
    qWarning() << "Error happened" << err << error;
}

void PackageKitUpdater::mediaChange(PackageKit::Transaction::MediaType media, const QString &type, const QString &text)
{
    Q_UNUSED(media)
    Q_EMIT passiveMessage(i18n("Media Change of type '%1' is requested.\n%2", type, text));
}

EulaHandling handleEula(const QString &eulaID, const QString &licenseAgreement)
{
    KConfigGroup group(KSharedConfig::openConfig(), "EULA");
    auto licenseGroup = group.group(eulaID);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(licenseAgreement.toUtf8());
    QByteArray hashHex = hash.result().toHex();

    EulaHandling ret;
    ret.request = licenseGroup.readEntry("Hash", QByteArray()) != hashHex;
    if (!ret.request) {
        ret.proceedFunction = [eulaID] {
            return PackageKit::Daemon::acceptEula(eulaID);
        };
    } else {
        ret.proceedFunction = [eulaID, hashHex] {
            KConfigGroup group(KSharedConfig::openConfig(), "EULA");
            KConfigGroup licenseGroup = group.group(eulaID);
            licenseGroup.writeEntry<QByteArray>("Hash", hashHex);
            return PackageKit::Daemon::acceptEula(eulaID);
        };
    }
    return ret;
}

void PackageKitUpdater::eulaRequired(const QString &eulaID, const QString &packageID, const QString &vendor, const QString &licenseAgreement)
{
    const auto handle = handleEula(eulaID, licenseAgreement);
    m_proceedFunctions << handle.proceedFunction;
    if (handle.request) {
        Q_EMIT proceedRequest(i18n("Accept EULA"),
                              i18n("The package %1 and its vendor %2 require that you accept their license:\n %3",
                                   PackageKit::Daemon::packageName(packageID),
                                   vendor,
                                   licenseAgreement));
    } else {
        proceed();
    }
}

void PackageKitUpdater::setProgressing(bool progressing)
{
    if (m_isProgressing != progressing) {
        m_isProgressing = progressing;
        Q_EMIT progressingChanged(m_isProgressing);
    }
}

void PackageKitUpdater::fetchLastUpdateTime()
{
    QDBusPendingReply<uint> transaction = PackageKit::Daemon::global()->getTimeSinceAction(PackageKit::Transaction::RoleGetUpdates);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(transaction, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PackageKitUpdater::lastUpdateTimeReceived);
}

void PackageKitUpdater::lastUpdateTimeReceived(QDBusPendingCallWatcher *w)
{
    QDBusPendingReply<uint> reply = w->reply();
    if (reply.isError()) {
        qWarning() << "Error when fetching the last update time" << reply.error();
    } else {
        m_lastUpdate = QDateTime::currentDateTime().addSecs(-int(reply.value()));
    }
    w->deleteLater();
}

AbstractBackendUpdater::State toUpdateState(PackageKit::Transaction::Status t)
{
    switch (t) {
    case PackageKit::Transaction::StatusUnknown:
    case PackageKit::Transaction::StatusDownload:
        return AbstractBackendUpdater::Downloading;
    case PackageKit::Transaction::StatusDepResolve:
    case PackageKit::Transaction::StatusSigCheck:
    case PackageKit::Transaction::StatusTestCommit:
    case PackageKit::Transaction::StatusInstall:
    case PackageKit::Transaction::StatusCommit:
        return AbstractBackendUpdater::Installing;
    case PackageKit::Transaction::StatusFinished:
    case PackageKit::Transaction::StatusCancel:
        return AbstractBackendUpdater::Done;
    default:
        qCDebug(LIBDISCOVER_BACKEND_LOG) << "unknown packagekit status" << t;
        return AbstractBackendUpdater::None;
    }
    Q_UNREACHABLE();
}

void PackageKitUpdater::itemProgress(const QString &itemID, PackageKit::Transaction::Status status, uint percentage)
{
    const auto res = packagesForPackageId({itemID});

    foreach (auto r, res) {
        Q_EMIT resourceProgressed(r, percentage, toUpdateState(status));
    }
}

void PackageKitUpdater::fetchChangelog() const
{
    QStringList pkgids;
    foreach (AbstractResource *res, m_allUpgradeable) {
        if (auto upgrade = dynamic_cast<SystemUpgrade *>(res)) {
            upgrade->fetchChangelog();
        } else {
            pkgids += static_cast<PackageKitResource *>(res)->availablePackageId();
        }
    }
    Q_ASSERT(!pkgids.isEmpty());

    PackageKit::Transaction *t = PackageKit::Daemon::getUpdatesDetails(pkgids);
    connect(t, &PackageKit::Transaction::updateDetail, this, &PackageKitUpdater::updateDetail);
    connect(t, &PackageKit::Transaction::errorCode, this, &PackageKitUpdater::errorFound);
}

void PackageKitUpdater::updateDetail(const QString &packageID,
                                     const QStringList &updates,
                                     const QStringList &obsoletes,
                                     const QStringList &vendorUrls,
                                     const QStringList &bugzillaUrls,
                                     const QStringList &cveUrls,
                                     PackageKit::Transaction::Restart restart,
                                     const QString &updateText,
                                     const QString &changelog,
                                     PackageKit::Transaction::UpdateState state,
                                     const QDateTime &issued,
                                     const QDateTime &updated)
{
    auto res = packagesForPackageId({packageID});
    foreach (auto r, res) {
        static_cast<PackageKitResource *>(r)
            ->updateDetail(packageID, updates, obsoletes, vendorUrls, bugzillaUrls, cveUrls, restart, updateText, changelog, state, issued, updated);
    }
}

void PackageKitUpdater::packageResolved(PackageKit::Transaction::Info info, const QString &packageId)
{
    m_packagesModified[info] << packageId;
}

void PackageKitUpdater::repoSignatureRequired(const QString &packageID,
                                              const QString &repoName,
                                              const QString &keyUrl,
                                              const QString &keyUserid,
                                              const QString &keyId,
                                              const QString &keyFingerprint,
                                              const QString &keyTimestamp,
                                              PackageKit::Transaction::SigType type)
{
    Q_EMIT proceedRequest(i18n("Missing signature for %1 in %2", packageID, repoName),
                          i18n("Do you trust the following key?\n\nUrl: %1\nUser: %2\nKey: %3\nFingerprint: %4\nTimestamp: %4\n",
                               keyUrl,
                               keyUserid,
                               keyFingerprint,
                               keyTimestamp));

    m_proceedFunctions << [type, keyId, packageID]() {
        return PackageKit::Daemon::installSignature(type, keyId, packageID);
    };
}

double PackageKitUpdater::updateSize() const
{
    double ret = 0.;
    QSet<QString> donePkgs;
    for (AbstractResource *res : m_toUpgrade) {
        if (auto upgrade = dynamic_cast<SystemUpgrade *>(res)) {
            ret += upgrade->size();
            continue;
        }

        PackageKitResource *app = qobject_cast<PackageKitResource *>(res);
        QString pkgid = m_backend->upgradeablePackageId(app);
        if (!donePkgs.contains(pkgid)) {
            donePkgs.insert(pkgid);
            ret += app->size();
        }
    }
    return ret;
}

quint64 PackageKitUpdater::downloadSpeed() const
{
    return m_transaction ? m_transaction->speed() : 0;
}
