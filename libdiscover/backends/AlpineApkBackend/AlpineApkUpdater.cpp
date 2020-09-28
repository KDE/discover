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
#include "alpineapk_backend_logging.h"
#include "utils.h"

#include <KAuthExecuteJob>
#include <kauth_version.h>
#include <KLocalizedString>

#include <QtApk>


AlpineApkUpdater::AlpineApkUpdater(AbstractResourcesBackend *parent)
    : AbstractBackendUpdater(parent)
    , m_backend(static_cast<AlpineApkBackend *>(parent))
{
    //
}

void AlpineApkUpdater::prepare()
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;

    QtApk::Database *db = m_backend->apkdb();

    if (db->isOpen()) {
        return;
    }

    // readonly is fine for a simulation of upgrade
    if (!db->open(QtApk::QTAPK_OPENF_READONLY)) {
        emit passiveMessage(i18n("Failed to open APK database!"));
        return;
    }

    if (!db->upgrade(QtApk::QTAPK_UPGRADE_SIMULATE, &m_upgradeable)) {
        emit passiveMessage(i18n("Failed to get a list of packages to upgrade!"));
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
    for (const QtApk::ChangesetItem &it : qAsConst(m_upgradeable.changes())) {
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
    // qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO << m_updatesCount;
    return (m_updatesCount > 0);
}

qreal AlpineApkUpdater::progress() const
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    return 0.0;
}

void AlpineApkUpdater::removeResources(const QList<AbstractResource *> &apps)
{
    // qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    const QSet<AbstractResource *> checkSet = kToSet(apps);
    m_markedToUpdate -= checkSet;
}

void AlpineApkUpdater::addResources(const QList<AbstractResource *> &apps)
{
    //Q_UNUSED(apps)
    //qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
    const QSet<AbstractResource *> checkSet = kToSet(apps);
    m_markedToUpdate += checkSet;
}

QList<AbstractResource *> AlpineApkUpdater::toUpdate() const
{
    // qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
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
    KAuth::Action upgradeAction(QStringLiteral("org.kde.discover.alpineapkbackend.upgrade"));
    upgradeAction.setHelperId(QStringLiteral("org.kde.discover.alpineapkbackend"));

    if (!upgradeAction.isValid()) {
        qCWarning(LOG_ALPINEAPK) << "kauth upgradeAction is not valid!";
        return;
    }

    upgradeAction.setTimeout(30 * 60 * 1000); // 30 min
#if KAUTH_VERSION < QT_VERSION_CHECK(5, 68, 0)
    upgradeAction.setDetails(i18n("Upgrade currently installed packages"));
#else
    static const KAuth::Action::DetailsMap details{
        { KAuth::Action::AuthDetail::DetailMessage, i18n("Upgrade currently installed packages") }
    };
    upgradeAction.setDetailsV2(details);
#endif
    // upgradeAction.addArgument(QLatin1String("onlySimulate"), true);

    // run upgrade with elevated privileges
    KAuth::ExecuteJob *reply = upgradeAction.execute();
    QObject::connect(reply, &KAuth::ExecuteJob::result,
                     this, &AlpineApkUpdater::handleKAuthUpgradeHelperReply);

    m_progressing = true;
    Q_EMIT progressingChanged(m_progressing);

    reply->start();
}

void AlpineApkUpdater::proceed()
{
    qCDebug(LOG_ALPINEAPK) << Q_FUNC_INFO;
}

int AlpineApkUpdater::updatesCount()
{
    // qDebug(LOG_ALPINEAPK) << Q_FUNC_INFO << m_updatesCount;
    return m_updatesCount;
}

void AlpineApkUpdater::startCheckForUpdates()
{
    QtApk::Database *db = m_backend->apkdb();
    KAuth::Action updateAction(QStringLiteral("org.kde.discover.alpineapkbackend.update"));
    updateAction.setHelperId(QStringLiteral("org.kde.discover.alpineapkbackend"));
    if (!updateAction.isValid()) {
        qCWarning(LOG_ALPINEAPK) << "kauth updateAction is not valid!";
        return;
    }
    updateAction.setTimeout(60 * 1000); // 1 minute
    // setDetails deprecated since KF 5.68, use setDetailsV2() with DetailsMap.
#if KAUTH_VERSION < QT_VERSION_CHECK(5, 68, 0)
    updateAction.setDetails(i18n("Update repositories index"));
#else
    static const KAuth::Action::DetailsMap details{
        { KAuth::Action::AuthDetail::DetailMessage, i18n("Update repositories index") }
    };
    updateAction.setDetailsV2(details);
#endif
    updateAction.addArgument(QLatin1String("fakeRoot"), db->fakeRoot());

    // run updates check with elevated privileges to access
    //     system package manager files
    KAuth::ExecuteJob *reply = updateAction.execute();
    QObject::connect(reply, &KAuth::ExecuteJob::result,
                     this, &AlpineApkUpdater::handleKAuthUpdateHelperReply);
    // qOverload is needed because of conflict with getter named percent()
    QObject::connect(reply, QOverload<KJob *, unsigned long>::of(&KAuth::ExecuteJob::percent),
                     this, &AlpineApkUpdater::handleKAuthUpdateHelperProgress);

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

void AlpineApkUpdater::handleKAuthUpgradeHelperReply(KJob *job)
{
    KAuth::ExecuteJob *reply = static_cast<KAuth::ExecuteJob *>(job);
    const QVariantMap &replyData = reply->data();
    if (reply->error() == 0) {
        QVariant pkgsV = replyData.value(QLatin1String("changes"));
        bool onlySimulate = replyData.value(QLatin1String("onlySimulate"), false).toBool();
        qCDebug(LOG_ALPINEAPK) << "KAuth helper upgrade reply received, onlySimulate:" << onlySimulate;
        if (onlySimulate) {
            QVector<QtApk::Package> pkgVector = pkgsV.value<QVector<QtApk::Package>>();
            qCDebug(LOG_ALPINEAPK) << "  num changes:" << pkgVector.size();
            for (const QtApk::Package &pkg : pkgVector) {
                qCDebug(LOG_ALPINEAPK)  << "    " << pkg.name << pkg.version;
            }
        }
    } else {
        handleKAuthHelperError(reply, replyData);
    }

    // we are not in the state "Fetching updates" now, update UI
    Q_EMIT checkForUpdatesFinished();
}

void AlpineApkUpdater::handleKAuthHelperError(
        KAuth::ExecuteJob *reply,
        const QVariantMap &replyData)
{
    const QString message = replyData.value(QLatin1String("errorString"),
                                            reply->errorString()).toString();
    qCDebug(LOG_ALPINEAPK) << "KAuth helper returned error:" << message << reply->error();
    if (reply->error() == KAuth::ActionReply::Error::AuthorizationDeniedError) {
        Q_EMIT passiveMessage(i18n("Authorization denied"));
    } else {
        Q_EMIT passiveMessage(i18n("Error") + QStringLiteral(":\n") + message);
    }
}

