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

#include <QEventLoop>
#include <QObject>
#include <QVariant>
#include <KAuthActionReply>

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
