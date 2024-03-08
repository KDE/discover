/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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

#include "AlpineApkUpdater.h"
#include "AlpineApkResource.h"
#include "AlpineApkBackend.h"
#include "AlpineApkAuthActionFactory.h"
#include "alpineapk_backend_logging.h"
#include "utils.h"

#include <KAuth/ExecuteJob>
#include <KLocalizedString>
#include <kcoreaddons_version.h>

#include <QtApk>

#include <utility>

AlpineApkUpdater::AlpineApkUpdater(AbstractResourcesBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_backend(static_cast<AlpineApkBackend *>(parent))
{
    //
}

void AlpineApkUpdater::prepare()
{
    QtApk::Database *db = m_backend->apkdb();

    if (db->isOpen()) {
        return;
    }

    // readonly is fine for a simulation of upgrade
    if (!db->open(QtApk::QTAPK_OPENF_READONLY)) {
        Q_EMIT passiveMessage(i18n("Failed to open APK database!"));
        return;
    }

    if (!db->upgrade(QtApk::QTAPK_UPGRADE_SIMULATE, &m_upgradeable)) {
        Q_EMIT passiveMessage(i18n("Failed to get a list of packages to upgrade!"));
        db->close();
        return;
    }
    // close DB ASAP
    db->close();

    m_updatesCount = m_upgradeable.changes().size();
    qCDebug(LOG_ALPINEAPK) << "updater: prepare: updates count" << m_updatesCount;

    m_allUpdateable.clear();
    m_markedToUpdate.clear();
    QHash<QString, AlpineApkResource *> *resources = m_backend->resourcesPtr();
    for (const QtApk::ChangesetItem &it : std::as_const(m_upgradeable.changes())) {
        const QtApk::Package &oldPkg = it.oldPackage;
        const QString newVersion = it.newPackage.version;
        AlpineApkResource *res = resources->value(oldPkg.name);
        if (res) {
            res->setAvailableVersion(newVersion);
            m_allUpdateable.insert(res);
            m_markedToUpdate.insert(res);
        }
    }

    // emitting this signal here leads to infinite recursion
    // emit updatesCountChanged(m_updatesCount);
}

bool AlpineApkUpdater::hasUpdates() const
{
    return (m_updatesCount > 0);
}

qreal AlpineApkUpdater::progress() const
{
    return m_upgradeProgress;
}

void AlpineApkUpdater::removeResources(const QList<AbstractResource *> &apps)
{
    const QSet<AbstractResource *> checkSet = kToSet(apps);
    m_markedToUpdate -= checkSet;
}

void AlpineApkUpdater::addResources(const QList<AbstractResource *> &apps)
{
    const QSet<AbstractResource *> checkSet = kToSet(apps);
    m_markedToUpdate += checkSet;
}

QList<AbstractResource *> AlpineApkUpdater::toUpdate() const
{
    return m_allUpdateable.values();
}

QDateTime AlpineApkUpdater::lastUpdate() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    return QDateTime();
}

bool AlpineApkUpdater::isCancelable() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    return false;
}

bool AlpineApkUpdater::isProgressing() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO << m_progressing;
    return m_progressing;
}

bool AlpineApkUpdater::isMarked(AbstractResource *res) const
{
    return m_markedToUpdate.contains(res);
}

void AlpineApkUpdater::fetchChangelog() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
}

double AlpineApkUpdater::updateSize() const
{
    double sum = 0.0;
    for (AbstractResource *res : m_markedToUpdate) {
        sum += res->size();
    }
    return sum;
}

quint64 AlpineApkUpdater::downloadSpeed() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    return 0;
}

void AlpineApkUpdater::cancel()
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
}

void AlpineApkUpdater::start()
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;

    // run upgrade with elevated privileges
    KAuth::ExecuteJob *reply = ActionFactory::createUpgradeAction();
    if (!reply) return;

    QObject::connect(reply, &KAuth::ExecuteJob::result,
                     this, &AlpineApkUpdater::handleKAuthUpgradeHelperReply);
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(5,80,0)
    // qOverload is needed because of conflict with getter named percent()
    QObject::connect(reply, QOverload<KJob *, unsigned long>::of(&KAuth::ExecuteJob::percent),
                     this, &AlpineApkUpdater::handleKAuthUpgradeHelperProgress);
#else
    QObject::connect(reply, &KAuth::ExecuteJob::percentChanged,
                     this, &AlpineApkUpdater::handleKAuthUpgradeHelperProgress);
#endif

    m_progressing = true;
    m_upgradeProgress = 0.0;
    Q_EMIT progressingChanged(m_progressing);

    reply->start();
}

void AlpineApkUpdater::proceed()
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
}

int AlpineApkUpdater::updatesCount()
{
    return m_updatesCount;
}

void AlpineApkUpdater::startCheckForUpdates()
{
    QtApk::Database *db = m_backend->apkdb();

    // run updates check with elevated privileges to access
    //     system package manager files
    KAuth::ExecuteJob *reply = ActionFactory::createUpdateAction(db->fakeRoot());
    if (!reply) return;
    QObject::connect(reply, &KAuth::ExecuteJob::result,
                     this, &AlpineApkUpdater::handleKAuthUpdateHelperReply);
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(5,80,0)
    // qOverload is needed because of conflict with getter named percent()
    QObject::connect(reply, QOverload<KJob *, unsigned long>::of(&KAuth::ExecuteJob::percent),
                     this, &AlpineApkUpdater::handleKAuthUpdateHelperProgress);
#else
    QObject::connect(reply, &KAuth::ExecuteJob::percentChanged,
                     this, &AlpineApkUpdater::handleKAuthUpdateHelperProgress);
#endif

    m_progressing = true;
    Q_EMIT progressingChanged(m_progressing);
    Q_EMIT progressChanged(0);

    reply->start();
}

void AlpineApkUpdater::handleKAuthUpdateHelperReply(KJob *job)
{
    KAuth::ExecuteJob *reply = static_cast<KAuth::ExecuteJob *>(job);
    const QVariantMap &replyData = reply->data();
    if (reply->error() == 0) {
        m_updatesCount = replyData.value(QLatin1String("updatesCount")).toInt();
        qCDebug(LOG_ALPINEAPK) << "KAuth helper update reply received, updatesCount:" << m_updatesCount;
        Q_EMIT updatesCountChanged(m_updatesCount);
    } else {
        handleKAuthHelperError(reply, replyData);
    }

    m_progressing = false;
    Q_EMIT progressingChanged(m_progressing);

    // we are not in the state "Fetching updates" now, update UI
    Q_EMIT checkForUpdatesFinished();
}

void AlpineApkUpdater::handleKAuthUpdateHelperProgress(KJob *job, unsigned long percent)
{
    Q_UNUSED(job)
    qCDebug(LOG_ALPINEAPK) << "    fetch updates progress: " << percent;
    Q_EMIT fetchingUpdatesProgressChanged(percent);
    Q_EMIT progressChanged(static_cast<qreal>(percent));
}

void AlpineApkUpdater::handleKAuthUpgradeHelperProgress(KJob *job, unsigned long percent)
{
    Q_UNUSED(job)
    qCDebug(LOG_ALPINEAPK) << "    upgrade progress: " << percent;
    qreal newProgress = static_cast<qreal>(percent);
    if (newProgress != m_upgradeProgress) {
        m_upgradeProgress = newProgress;
        Q_EMIT progressChanged(m_upgradeProgress);
    }
}

void AlpineApkUpdater::handleKAuthUpgradeHelperReply(KJob *job)
{
    KAuth::ExecuteJob *reply = static_cast<KAuth::ExecuteJob *>(job);
    const QVariantMap &replyData = reply->data();
    if (reply->error() == 0) {
        QVariant pkgsV = replyData.value(QLatin1String("changes"));
        bool onlySimulate = replyData.value(QLatin1String("onlySimulate"), false).toBool();
        if (onlySimulate) {
            qCDebug(LOG_ALPINEAPK) << "KAuth helper upgrade reply received, simulation mode";
            QVector<QtApk::Package> pkgVector = pkgsV.value<QVector<QtApk::Package>>();
            qCDebug(LOG_ALPINEAPK) << "  num changes:" << pkgVector.size();
            for (const QtApk::Package &pkg : pkgVector) {
                qCDebug(LOG_ALPINEAPK)  << "    " << pkg.name << pkg.version;
            }
        }
    } else {
        handleKAuthHelperError(reply, replyData);
    }

    m_progressing = false;
    Q_EMIT progressingChanged(m_progressing);
}

void AlpineApkUpdater::handleKAuthHelperError(
        KAuth::ExecuteJob *reply,
        const QVariantMap &replyData)
{
    // error message should be received as part of JSON reply from helper
    QString message = replyData.value(QLatin1String("errorString"),
                                            reply->errorString()).toString();
    if (reply->error() == KAuth::ActionReply::Error::AuthorizationDeniedError) {
        qCWarning(LOG_ALPINEAPK) << "updater: KAuth helper returned AuthorizationDeniedError";
        Q_EMIT passiveMessage(i18n("Authorization denied"));
    } else {
        // if received error message is empty, try other ways to get error text for user
        // there are multiple ways to get error messages in kauth/kjob
        if (message.isEmpty()) {
            message = reply->errorString();
            if (message.isEmpty()) {
                message = reply->errorText();
            }
        }
        qCDebug(LOG_ALPINEAPK) << "updater: KAuth helper returned error:" << message << reply->error();
        Q_EMIT passiveMessage(i18n("Error") + QStringLiteral(":\n") + message);
    }
}

