/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ApplicationBackend.h"

#include <QtCore/QDir>
#include <QtCore/QStringList>

#include <KConfigGroup>
#include <KLocale>
#include <KMessageBox>
#include <KService>

#include <LibQApt/Backend>
#include <DebconfGui.h>

#include "Application.h"
#include "ApplicationLauncher.h"
#include "Transaction.h"

ApplicationBackend::ApplicationBackend(QObject *parent)
    : QObject(parent)
    , m_backend(0)
    , m_appLauncher(0)
{
    m_currentTransaction = m_queue.end();

    m_pkgBlacklist << "kdebase-runtime" << "kdepim-runtime" << "kdelibs5-plugins";
}

ApplicationBackend::~ApplicationBackend()
{
}

void ApplicationBackend::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_backend->setUndoRedoCacheSize(1);
    init();

    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode, const QVariantMap &)),
            this, SLOT(errorOccurred(QApt::ErrorCode, const QVariantMap &)));
}

void ApplicationBackend::init()
{
    QList<int> popconScores;
    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QDir::Files);
    foreach(const QString &fileName, fileList) {
        Application *app = new Application("/usr/share/app-install/desktop/" + fileName, m_backend);
        if (app->isValid()) {
            if (app->package() && !m_pkgBlacklist.contains(app->package()->latin1Name())) {
                m_appList << app;
                popconScores << app->popconScore();
            }
        } else {
            // Invalid .desktop file
            // kDebug() << fileName;
        }
    }
    qSort(popconScores);

    m_maxPopconScore = popconScores.last();
    qDeleteAll(m_queue);
    m_queue.clear();
}

void ApplicationBackend::reload()
{
    emit reloadStarted();
    qDeleteAll(m_appList);
    m_appList.clear();
    m_queue.clear();
    m_backend->reloadCache();

    init();

    // We must init() before showing the app launcher, otherwise we
    // won't be able to get installed file info on the newly-installed
    // packages
    KConfig config("muon-installerrc");
    KConfigGroup notifyGroup(&config, "Notification Messages");
    bool show = notifyGroup.readEntry("ShowApplicationLauncher", true);

    if (show) {
        showAppLauncher();
    }

    emit reloadFinished();
}

void ApplicationBackend::workerEvent(QApt::WorkerEvent event)
{
    m_workerState.first = event;

    Transaction *transaction = 0;
    if (!m_queue.isEmpty()) {
        m_workerState.second = (*m_currentTransaction);
        transaction = (*m_currentTransaction);
    }

    emit workerEvent(event, transaction);

    switch (event) {
    case QApt::PackageDownloadStarted:
        (*m_currentTransaction)->setState(RunningState);
        connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                   this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::CommitChangesStarted:
        m_debconfGui = new DebconfKde::DebconfGui("/tmp/qapt-sock");
        (*m_currentTransaction)->setState(RunningState);
        connect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                this, SLOT(updateCommitProgress(const QString &, int)));
        m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
        m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
        break;
    case QApt::CommitChangesFinished:
        disconnect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                   this, SLOT(updateCommitProgress(const QString &, int)));

        if (m_currentTransaction != m_queue.end()) {
            m_appLaunchQueue << (*m_currentTransaction)->application()->package()->name();
        }

        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
        (*m_currentTransaction)->setState(DoneState);
        ++m_currentTransaction;

        if (m_currentTransaction == m_queue.end()) {
            reload();
        } else {
            runNextTransaction();
        }
        break;
    default:
        break;
    }
}

void ApplicationBackend::errorOccurred(QApt::ErrorCode error, const QVariantMap &details)
{
    Q_UNUSED(details);

    disconnect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                   this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                   this, SLOT(updateCommitProgress(const QString &, int)));

    // Undo marking if an AuthError is encountered, since our install/remove
    // buttons do both marking and committing
    switch (error) {
    case QApt::AuthError:
        emit transactionCancelled((*m_currentTransaction)->application());
        m_backend->undo();
        break;
    case QApt::UserCancelError:
        // Handled in transactionCancelled()
        return;
    default:
        break;
    }

    // Reset worker state on failure
    if (m_workerState.second) {
        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
    }

    // A CommitChangesFinished signal will still be fired in this case,
    // and workerEvent will take care of this for us
    if (error != QApt::CommitError) {
        ++m_currentTransaction;
    }
}

void ApplicationBackend::updateDownloadProgress(int percentage)
{
    emit progress(*m_currentTransaction, percentage);
}

void ApplicationBackend::updateCommitProgress(const QString &text, int percentage)
{
    Q_UNUSED(text);

    emit progress(*m_currentTransaction, percentage);
}

void ApplicationBackend::addTransaction(Transaction *transaction)
{
    transaction->setState(QueuedState);
    m_queue.append(transaction);

    if (m_queue.count() == 1) {
        m_currentTransaction = m_queue.begin();
        runNextTransaction();
    }
}

void ApplicationBackend::cancelTransaction(Application *app)
{
    disconnect(m_backend, SIGNAL(downloadProgress(int, int, int)),
               this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(const QString &, int)),
               this, SLOT(updateCommitProgress(const QString &, int)));
    QList<Transaction *>::iterator iter = m_queue.begin();

    while (iter != m_queue.end()) {
        if ((*iter)->application() == app) {
            if ((*iter)->state() == RunningState) {
                m_backend->cancelDownload();
                m_backend->undo();
            }
            m_queue.erase(iter);
            break;
        }
        ++iter;
    }

    emit transactionCancelled(app);
}

void ApplicationBackend::runNextTransaction()
{
    QApt::CacheState oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();

    Application *app = (*m_currentTransaction)->application();

    switch ((*m_currentTransaction)->action()) {
    case InstallApp:
        app->package()->setInstall();
        break;
    case RemoveApp:
        app->package()->setRemove();
        break;
    default:
        break;
    }

    QHash<QApt::Package *, QApt::Package::State> addons = (*m_currentTransaction)->addons();
    QHash<QApt::Package *, QApt::Package::State>::const_iterator iter = addons.constBegin();

    while (iter != addons.constEnd()) {
        switch (iter.value()) {
        case QApt::Package::ToInstall:
            iter.key()->setInstall();
            break;
        case QApt::Package::ToRemove:
            iter.key()->setRemove();
            break;
        default:
            break;
        }
        ++iter;
    }

    if (app->package()->wouldBreak()) {
        m_backend->restoreCacheState(oldCacheState);
        //TODO Notify of error
    }

    m_backend->commitChanges();
}

void ApplicationBackend::showAppLauncher()
{
    QApt::PackageList packages;

    foreach (const QString &package, m_appLaunchQueue) {
        QApt::Package *pkg = m_backend->package(package);
        if (pkg) {
            packages << pkg;
        }
    }

    m_appLaunchQueue.clear();

    QVector<KService*> apps;
    foreach (QApt::Package *package, packages) {
        if (!package->isInstalled()) {
            continue;
        }

        // TODO: move to Application (perhaps call it Application::executables())
        foreach (const QString &desktop, package->installedFilesList().filter(".desktop")) {
            // we create a new KService because findByDestopPath
            // might fail because the Sycoca database is not up to date yet.
            KService *service = new KService(desktop);
            if (service->isApplication() &&
              !service->noDisplay() &&
              !service->exec().isEmpty())
            {
                apps << service;
            }
        }
    }

    if (!m_appLauncher && !apps.isEmpty()) {
        m_appLauncher = new ApplicationLauncher(apps);
        connect(m_appLauncher, SIGNAL(destroyed(QObject *)),
            this, SLOT(onAppLauncherClosed()));
        connect(m_appLauncher, SIGNAL(finished(int)),
            this, SLOT(onAppLauncherClosed()));
        m_appLauncher->setWindowTitle(i18nc("@title:window", "Installation Complete"));
        m_appLauncher->show();
    }
}

void ApplicationBackend::onAppLauncherClosed()
{
    m_appLauncher->deleteLater();
    m_appLauncher = 0;
}

QList<Application *> ApplicationBackend::applicationList() const
{
    return m_appList;
}

QPair<QApt::WorkerEvent, Transaction *> ApplicationBackend::workerState() const
{
    return m_workerState;
}

QList<Transaction *> ApplicationBackend::transactions() const
{
    return m_queue;
}

int ApplicationBackend::maxPopconScore() const
{
    return m_maxPopconScore;
}
