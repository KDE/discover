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

#include "AlpineApkAuthHelper.h"

#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtDebugMsg)
#else
Q_LOGGING_CATEGORY(LOG_AUTHHELPER, "org.kde.discover.alpineapkbackend.authhelper", QtWarningMsg)
#endif

using namespace KAuth;

AlpineApkAuthHelper::AlpineApkAuthHelper() {}

ActionReply AlpineApkAuthHelper::test(const QVariantMap &args)
{
    const QString txt = args[QStringLiteral("txt")].toString();

    ActionReply reply = ActionReply::HelperErrorReply();
    QByteArray replyData(QByteArrayLiteral("ok"));

    // write some text file at the root directory as root, why not
    QFile f(QStringLiteral("/lol.txt"));
    if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
        f.write(txt.toUtf8());
        f.close();

        reply = ActionReply::SuccessReply();
        reply.setData({
            { QStringLiteral("reply"), replyData },
        });
    }

    return reply;
}

ActionReply AlpineApkAuthHelper::update(const QVariantMap &args)
{
    ActionReply reply = ActionReply::HelperErrorReply();

    const QString fakeRoot = args.value(QLatin1String("fakeRoot"), QString()).toString();
    if (!fakeRoot.isEmpty()) {
        m_apkdb.setFakeRoot(fakeRoot);
    }

    HelperSupport::progressStep(10);

    if (!m_apkdb.open(QtApk::Database::QTAPK_OPENF_READWRITE)) {
        reply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return reply;
    }

    bool update_ok = m_apkdb.updatePackageIndex();

    if (update_ok) {
        int updatesCount = m_apkdb.upgradeablePackagesCount();
        reply = ActionReply::SuccessReply();
        reply.setData({
            { QLatin1String("updatesCount"), updatesCount }
        });
    } else {
        reply.setErrorDescription(QStringLiteral("Repo update failed!"));
        reply.setData({
            { QLatin1String("errorString"), QStringLiteral("Repo update failed!") }
        });
    }

    m_apkdb.close();
    HelperSupport::progressStep(100);

    return reply;
}

ActionReply AlpineApkAuthHelper::add(const QVariantMap &args)
{
    ActionReply reply = ActionReply::HelperErrorReply();
    return reply;
}

ActionReply AlpineApkAuthHelper::del(const QVariantMap &args)
{
    ActionReply reply = ActionReply::HelperErrorReply();
    return reply;
}

ActionReply AlpineApkAuthHelper::upgrade(const QVariantMap &args)
{
    ActionReply reply = ActionReply::HelperErrorReply();

    HelperSupport::progressStep(10);

    if (!m_apkdb.open(QtApk::Database::QTAPK_OPENF_READWRITE)) {
        reply.setErrorDescription(QStringLiteral("Failed to open database!"));
        return reply;
    }

    bool onlySimulate = args.value(QLatin1String("onlySimulate"), false).toBool();
    QtApk::Database::DbUpgradeFlags flags = QtApk::Database::QTAPK_UPGRADE_DEFAULT;
    if (onlySimulate) {
        flags = QtApk::Database::QTAPK_UPGRADE_SIMULATE;
        qCDebug(LOG_AUTHHELPER) << "Simulating upgrade run.";
    }

    const QString fakeRoot = args.value(QLatin1String("fakeRoot"), QString()).toString();
    if (!fakeRoot.isEmpty()) {
        m_apkdb.setFakeRoot(fakeRoot);
    }

    int progress_fd = m_apkdb.progressFd();
    qCDebug(LOG_AUTHHELPER) << "    progress_fd: " << progress_fd;

    QScopedPointer<QSocketNotifier> notifier(new QSocketNotifier(progress_fd, QSocketNotifier::Read));
    QObject::connect(notifier.data(), &QSocketNotifier::activated, notifier.data(), [](int sock) {
        qCDebug(LOG_AUTHHELPER) << "        read trigger from progress_fd!";
    });

    QtApk::Changeset changes;
    bool upgrade_ok = m_apkdb.upgrade(flags, &changes);

    if (upgrade_ok) {
        reply = ActionReply::SuccessReply();
        QVariantMap replyData;
        const QVector<QtApk::ChangesetItem> ch = changes.changes();
        QVector<QVariant> chVector;
        QVector<QtApk::Package> pkgVector;
        for (const QtApk::ChangesetItem &it: ch) {
            pkgVector << it.newPackage;
        }
        replyData.insert(QLatin1String("changes"), QVariant::fromValue(pkgVector));
        replyData.insert(QLatin1String("onlySimulate"), onlySimulate);
        reply.setData(replyData);
    } else {
        reply.setErrorDescription(QStringLiteral("Repo upgrade failed!"));
    }

    m_apkdb.close();
    HelperSupport::progressStep(100);

    return reply;
}

KAUTH_HELPER_MAIN("org.kde.discover.alpineapkbackend", AlpineApkAuthHelper)
