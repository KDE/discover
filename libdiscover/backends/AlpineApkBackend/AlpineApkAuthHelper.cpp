/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QProcess>
#include <QScopedPointer>
#include <QSocketNotifier>

#include <KAuth/HelperSupport>
#include <kauth_version.h>

#include "AlpineApkAuthHelper.h"

#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtDebugMsg)
#else
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtWarningMsg)
#endif

using namespace KAuth;

AlpineApkAuthHelper::AlpineApkAuthHelper()
{
}

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
    QObject::connect(trans, &QtApk::Transaction::progressChanged, this, &AlpineApkAuthHelper::reportProgress);

    // receive error messages
    QObject::connect(trans, &QtApk::Transaction::errorOccured, this, &AlpineApkAuthHelper::onTransactionError);

    // what to do when transaction is complete
    QObject::connect(trans, &QtApk::Transaction::finished, this, &AlpineApkAuthHelper::onTransactionFinished);

    if (!m_loop) {
        m_loop = new QEventLoop(this);
    }
}

void AlpineApkAuthHelper::reportProgress(float percent)
{
    int p = static_cast<int>(percent);
    if (p < 0)
        p = 0;
    if (p > 100)
        p = 100;
    HelperSupport::progressStep(p);
}

void AlpineApkAuthHelper::onTransactionError(const QString &msg)
{
    qCWarning(LOG_AUTHHELPER).nospace() << "ERROR occurred in transaction \"" << m_currentTransaction->desc() << "\": " << msg;
    // construct error message to use in helper reply
    const QString errMsg = m_currentTransaction->desc() + QLatin1String(" failed: ") + msg;
    m_actionReply.setErrorDescription(errMsg);
    m_actionReply.setData({{QLatin1String("errorString"), errMsg}});
    m_trans_ok = false;
}

void AlpineApkAuthHelper::onTransactionFinished()
{
    m_lastChangeset = m_currentTransaction->changeset();
    m_currentTransaction->deleteLater();
    m_currentTransaction = nullptr;
    m_loop->quit();
}

// single entry point for all package management actions
ActionReply AlpineApkAuthHelper::pkgmgmt(const QVariantMap &args)
{
    m_actionReply = ActionReply::HelperErrorReply();
    HelperSupport::progressStep(0);

    // actual package management action to perform is passed in "pkgAction" argument
    if (!args.contains(QLatin1String("pkgAction"))) {
        m_actionReply.setError(ActionReply::InvalidActionError);
        m_actionReply.setErrorDescription(QLatin1String("Please pass \'pkgAction\' argument."));
        HelperSupport::progressStep(100);
        return m_actionReply;
    }

    const QString pkgAction = args.value(QLatin1String("pkgAction")).toString();

    if (pkgAction == QStringLiteral("update")) {
        update(args);
    } else if (pkgAction == QStringLiteral("add")) {
        add(args);
    } else if (pkgAction == QStringLiteral("del")) {
        del(args);
    } else if (pkgAction == QStringLiteral("upgrade")) {
        upgrade(args);
    } else if (pkgAction == QStringLiteral("repoconfig")) {
        repoconfig(args);
    } else {
        // error: unknown pkgAction
        m_actionReply.setError(ActionReply::NoSuchActionError);
        m_actionReply.setErrorDescription(QLatin1String("Please pass a valid \'pkgAction\' argument. "
                                                        "Action \"%1\" is not recognized.")
                                              .arg(pkgAction));
    }

    HelperSupport::progressStep(100);
    return m_actionReply;
}

void AlpineApkAuthHelper::update(const QVariantMap &args)
{
    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return;
    }

    m_trans_ok = true;
    QtApk::Transaction *trans = m_apkdb.updatePackageIndex();
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        int updatesCount = m_apkdb.upgradeablePackagesCount();
        m_actionReply = ActionReply::SuccessReply();
        m_actionReply.setData({{QLatin1String("updatesCount"), updatesCount}});
    }
}

void AlpineApkAuthHelper::add(const QVariantMap &args)
{
    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return;
    }

    const QString pkgName = args.value(QLatin1String("pkgName"), QString()).toString();
    if (pkgName.isEmpty()) {
        m_actionReply.setErrorDescription(QStringLiteral("Specify pkgName for adding!"));
        return;
    }

    m_trans_ok = true;
    QtApk::Transaction *trans = m_apkdb.add(pkgName);
    setupTransactionPostCreate(trans);

    trans->start();
    m_loop->exec();

    if (m_trans_ok) {
        m_actionReply = ActionReply::SuccessReply();
    }
}

void AlpineApkAuthHelper::del(const QVariantMap &args)
{
    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return;
    }

    const QString pkgName = args.value(QLatin1String("pkgName"), QString()).toString();
    if (pkgName.isEmpty()) {
        m_actionReply.setErrorDescription(QStringLiteral("Specify pkgName for removing!"));
        return;
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
}

void AlpineApkAuthHelper::upgrade(const QVariantMap &args)
{
    if (!openDatabase(args)) {
        m_actionReply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return;
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
        for (const QtApk::ChangesetItem &it : ch) {
            pkgVector << it.newPackage;
        }
        replyData.insert(QLatin1String("changes"), QVariant::fromValue(pkgVector));
        replyData.insert(QLatin1String("onlySimulate"), onlySimulate);
        m_actionReply.setData(replyData);
    }
}

void AlpineApkAuthHelper::repoconfig(const QVariantMap &args)
{
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
}

KAUTH_HELPER_MAIN("org.kde.discover.alpineapkbackend", AlpineApkAuthHelper)
