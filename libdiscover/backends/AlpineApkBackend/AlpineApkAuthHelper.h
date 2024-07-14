/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QEventLoop>
#include <QObject>
#include <QVariant>

#include <KAuth/ActionReply>

#include <QtApk>

using namespace KAuth;

class AlpineApkAuthHelper : public QObject
{
    Q_OBJECT
public:
    AlpineApkAuthHelper();
    ~AlpineApkAuthHelper() override;

public Q_SLOTS:
    // single entry point for all package management operations
    ActionReply pkgmgmt(const QVariantMap &args);

protected:
    // helpers
    bool openDatabase(const QVariantMap &args, bool readwrite = true);
    void closeDatabase();
    void setupTransactionPostCreate(QtApk::Transaction *trans);

    // individual pakckage management actions
    void update(const QVariantMap &args);
    void add(const QVariantMap &args);
    void del(const QVariantMap &args);
    void upgrade(const QVariantMap &args);
    void repoconfig(const QVariantMap &args);

protected Q_SLOTS:
    void reportProgress(float percent);
    void onTransactionError(const QString &msg);
    void onTransactionFinished();

private:
    QtApk::DatabaseAsync m_apkdb; // runs transactions in bg thread
    QtApk::Transaction *m_currentTransaction = nullptr;
    QEventLoop *m_loop = nullptr; // event loop that will run and wait while bg transaction is in progress
    ActionReply m_actionReply; // return value for main action slots
    bool m_trans_ok = true; // flag to indicate if bg transaction was successful
    QtApk::Changeset m_lastChangeset; // changeset from last completed transaction
};
