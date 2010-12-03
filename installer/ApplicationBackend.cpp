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
#include <KDebug>

#include <LibQApt/Backend>
#include <DebconfGui.h>

#include "Application.h"
#include "ApplicationLauncher.h"

ApplicationBackend::ApplicationBackend(QObject *parent)
    : QObject(parent)
    , m_backend(0)
    , m_appLauncher(0)
{
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
            m_appList << app;
            popconScores << app->popconScore();
        } else {
            // Invalid .desktop file
            // kDebug() << fileName;
        }
    }
    qSort(popconScores);

    m_maxPopconScore = popconScores.last();
    m_queue.clear();
}

void ApplicationBackend::reload()
{
    qDeleteAll(m_appList);
    m_appList.clear();
    m_queue.clear();
    m_backend->reloadCache();

    init();

    KConfig config("muon-installerrc");
    KConfigGroup notifyGroup(&config, "Notification Messages");
    bool show = notifyGroup.readEntry("ShowApplicationLauncher", true);

    if (show) {
        showAppLauncher();
    }

    emit reloaded();
}

void ApplicationBackend::workerEvent(QApt::WorkerEvent event)
{
    Application *app = 0;
    m_workerState.first = event;
    if (!m_queue.isEmpty()) {
        m_workerState.second = m_queue.first().application;
        app = m_queue.first().application;
    }

    emit workerEvent(event, app);

    switch (event) {
    case QApt::PackageDownloadStarted:
        m_queue.first().state = RunningState;
        connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                   this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::CommitChangesStarted:
        m_debconfGui = new DebconfKde::DebconfGui("/tmp/qapt-sock");
        m_queue.first().state = RunningState;
        connect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                this, SLOT(updateCommitProgress(const QString &, int)));
        m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
        m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
        break;
    case QApt::CommitChangesFinished:
        disconnect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                   this, SLOT(updateCommitProgress(const QString &, int)));

        m_appLaunchQueue << m_queue.first().application->package()->latin1Name();

        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
        m_queue.removeFirst();

        if (m_queue.isEmpty()) {
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
        emit transactionCancelled(m_queue.first().application);
        m_backend->undo();
        break;
    case QApt::UserCancelError:
        // Handled in transactionCancelled()
        return;
    default:
        break;
    }

    // Remove running transaction on failure
    if (m_workerState.second) {
        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
    }

    if (!m_queue.isEmpty()) {
        m_queue.removeFirst(); 
    }
}

void ApplicationBackend::updateDownloadProgress(int percentage)
{
    Application *app = m_queue.first().application;
    emit progress(app, percentage);
}

void ApplicationBackend::updateCommitProgress(const QString &text, int percentage)
{
    Q_UNUSED(text);

    Application *app = m_queue.first().application;
    emit progress(app, percentage);
}

void ApplicationBackend::addTransaction(Transaction transaction)
{
    transaction.state = QueuedState;
    m_queue.append(transaction);

    if (m_queue.count() == 1) {
        runNextTransaction();
    }
}

void ApplicationBackend::cancelTransaction(Application *app)
{
    disconnect(m_backend, SIGNAL(downloadProgress(int, int, int)),
               this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(const QString &, int)),
               this, SLOT(updateCommitProgress(const QString &, int)));
    QList<Transaction>::iterator iter = m_queue.begin();

    while (iter != m_queue.end()) {
        if ((*iter).application == app) {
            if ((*iter).state == RunningState) {
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

    Application *app = m_queue.first().application;

    switch (m_queue.first().action) {
    case QApt::Package::ToInstall:
    case QApt::Package::ToUpgrade:
        app->package()->setInstall();
        break;
    case QApt::Package::ToRemove:
        app->package()->setRemove();
        break;
    default:
        break;
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

    foreach (const QLatin1String &package, m_appLaunchQueue) {
        QApt::Package *pkg = m_backend->package(package);
        if (pkg) {
            packages << pkg;
        }
    }

    m_appLaunchQueue.clear();

    foreach (QApt::Package *package, packages) {
        if (!package->isInstalled()) {
            return;
        }

        QVector<KService*> apps;

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

        if (!m_appLauncher) {
            m_appLauncher = new ApplicationLauncher(apps);
            connect(m_appLauncher, SIGNAL(destroyed(QObject *)),
                this, SLOT(onAppLauncherClosed()));
            connect(m_appLauncher, SIGNAL(finished(int)),
                this, SLOT(onAppLauncherClosed()));
            m_appLauncher->setWindowTitle(i18nc("@title:window", "Installation Complete"));
            m_appLauncher->show();
        }
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

QPair<QApt::WorkerEvent, Application *> ApplicationBackend::workerState() const
{
    return m_workerState;
}

Transaction ApplicationBackend::currentTransaction() const
{
    Transaction transaction= {0, 0, InvalidState};
    if (!m_queue.isEmpty())
    {
        transaction = m_queue.first();
    }
    return transaction;
}

int ApplicationBackend::maxPopconScore() const
{
    return m_maxPopconScore;
}
