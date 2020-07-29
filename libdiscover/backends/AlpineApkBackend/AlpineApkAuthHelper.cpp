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

#include <QProcess>
#include <QDebug>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QScopedPointer>
#include <QFile>

#include <KAuthHelperSupport>
#include <kauth_version.h>

#include "AlpineApkAuthHelper.h"

#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtDebugMsg)
#else
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtWarningMsg)
#endif

using namespace KAuth;

AlpineApkAuthHelper::AlpineApkAuthHelper() {}

AlpineApkAuthHelper::~AlpineApkAuthHelper()
{
    closeDatabase();
}

bool AlpineApkAuthHelper::openDatabase(const QVariantMap &args, bool readwrite)
{
    // is already opened?
    if (m_apkdb.isOpen()) {
        return true;
    }

    // maybe set fakeRoot (needs to be done before Database::open()
    const QString fakeRoot = args.value(QLatin1String("fakeRoot"), QString()).toString();
    if (!fakeRoot.isEmpty()) {
        m_apkdb.setFakeRoot(fakeRoot);
    }

    // calculate flags to use during open
    QtApk::DbOpenFlags fl = QtApk::QTAPK_OPENF_ENABLE_PROGRESSFD;
    if (readwrite) {
        fl |= QtApk::QTAPK_OPENF_READWRITE;
    }

    if (!m_apkdb.open(fl)) {
        return false;
    }
    return true;
}

void AlpineApkAuthHelper::closeDatabase()
{
    // close database only if opened
    if (m_apkdb.isOpen()) {
        // this also stops bg thread
        m_apkdb.close();
    }
}

void AlpineApkAuthHelper::setupTransactionPostCreate(QtApk::Transaction *trans)
{
    m_currentTransaction = trans; // remember current transaction here

    // receive progress notifications
    QObject::connect(trans, &QtApk::Transaction::progressChanged,
                     this, &AlpineApkAuthHelper::reportProgress);

    // receive error messages
    QObject::connect(trans, &QtApk::Transaction::errorOccured,
                     this, &AlpineApkAuthHelper::onTransactionError);

    // what to do when transaction is complete
    QObject::connect(trans, &QtApk::Transaction::finished,
                     this, &AlpineApkAuthHelper::onTransactionFinished);

    if (!m_loop) {
        m_loop = new QEventLoop(this);
    }
}

void AlpineApkAuthHelper::reportProgress(float percent)
{
    int p = static_cast<int>(percent);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    HelperSupport::progressStep(p);
}

void AlpineApkAuthHelper::onTransactionError(const QString &msg)
{
    qCWarning(LOG_AUTHHELPER).nospace() << "ERROR occured in transaction \""
                                        << m_currentTransaction->desc()
                                        << "\": " << msg;
    const QString errMsg = m_currentTransaction->desc() + QLatin1String(" failed: ") + msg;
    m_actionReply.setErrorDescription(errMsg);
    m_actionReply.setData({
        { QLatin1String("errorString"), errMsg }
    });
    m_trans_ok = false;
}

void AlpineApkAuthHelper::onTransactionFinished()
{
    m_lastChangeset = m_currentTransaction->changeset();
    m_currentTransaction->deleteLater();
    m_currentTransaction = nullptr;
    m_loop->quit();
}

ActionReply AlpineApkAuthHelper::update(const QVariantMap &args)
{
    // return error by default
    m_actionReply = ActionReply::HelperErrorReply();

    HelperSupport::progressStep(0);

    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return m_actionReply;
    }

    m_trans_ok = true;
    QtApk::Transaction *trans = m_apkdb.updatePackageIndex();
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        int updatesCount = m_apkdb.upgradeablePackagesCount();
        m_actionReply = ActionReply::SuccessReply();
        m_actionReply.setData({
            { QLatin1String("updatesCount"), updatesCount }
        });
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

ActionReply AlpineApkAuthHelper::add(const QVariantMap &args)
{
    // return error by default
    m_actionReply = ActionReply::HelperErrorReply();

    HelperSupport::progressStep(0);

    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return m_actionReply;
    }

    const QString pkgName = args.value(QLatin1String("pkgName"), QString()).toString();
    if (pkgName.isEmpty()) {
        m_actionReply.setErrorDescription(QStringLiteral("Specify pkgName for adding!"));
        return m_actionReply;
    }

    m_trans_ok = true;
    QtApk::Transaction *trans = m_apkdb.add(pkgName);
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        m_actionReply = ActionReply::SuccessReply();
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

ActionReply AlpineApkAuthHelper::del(const QVariantMap &args)
{
    // return error by default
    m_actionReply = ActionReply::HelperErrorReply();

    HelperSupport::progressStep(0);

    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return m_actionReply;
    }

    const QString pkgName = args.value(QLatin1String("pkgName"), QString()).toString();
    if (pkgName.isEmpty()) {
        m_actionReply.setErrorDescription(QStringLiteral("Specify pkgName for removing!"));
        return m_actionReply;
    }

    const bool delRdepends = args.value(QLatin1String("delRdepends"), false).toBool();

    QtApk::DbDelFlags delFlags = QtApk::QTAPK_DEL_DEFAULT;
    if (delRdepends) {
        delFlags = QtApk::QTAPK_DEL_RDEPENDS;
    }

    m_trans_ok = true;
    QtApk::Transaction *trans = m_apkdb.del(pkgName, delFlags);
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        m_actionReply = ActionReply::SuccessReply();
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

ActionReply AlpineApkAuthHelper::upgrade(const QVariantMap &args)
{
    m_actionReply = ActionReply::HelperErrorReply();

    HelperSupport::progressStep(0);

    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return m_actionReply;
    }

    bool onlySimulate = args.value(QLatin1String("onlySimulate"), false).toBool();
    QtApk::DbUpgradeFlags flags = QtApk::QTAPK_UPGRADE_DEFAULT;
    if (onlySimulate) {
        flags = QtApk::QTAPK_UPGRADE_SIMULATE;
        qCDebug(LOG_AUTHHELPER) << "Simulating upgrade run.";
    }

    m_trans_ok = true;

    QtApk::Transaction *trans = m_apkdb.upgrade(flags);
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        m_actionReply = ActionReply::SuccessReply();
        QVariantMap replyData;
        const QVector<QtApk::ChangesetItem> ch = m_lastChangeset.changes();
        QVector<QVariant> chVector;
        QVector<QtApk::Package> pkgVector;
        for (const QtApk::ChangesetItem &it: ch) {
            pkgVector << it.newPackage;
        }
        replyData.insert(QLatin1String("changes"), QVariant::fromValue(pkgVector));
        replyData.insert(QLatin1String("onlySimulate"), onlySimulate);
        m_actionReply.setData(replyData);
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

ActionReply AlpineApkAuthHelper::repoconfig(const QVariantMap &args)
{
    m_actionReply = ActionReply::HelperErrorReply();
    HelperSupport::progressStep(10);

    if (args.contains(QLatin1String("repoList"))) {
        const QVariant v = args.value(QLatin1String("repoList"));
        const QVector<QtApk::Repository> repoVec = v.value<QVector<QtApk::Repository>>();
        if (QtApk::Database::saveRepositories(repoVec)) {
            m_actionReply = ActionReply::SuccessReply(); // OK
        } else {
            m_actionReply.setErrorDescription(QStringLiteral("Failed to write repositories config!"));
        }
    } else {
        m_actionReply.setErrorDescription(QStringLiteral("repoList parameter is missing in request!"));
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

KAUTH_HELPER_MAIN("org.kde.discover.alpineapkbackend", AlpineApkAuthHelper)
