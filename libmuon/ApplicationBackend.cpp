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

// Qt includes
#include <QtCore/QDir>
#include <QtCore/QStringList>

// KDE includes
#include <KLocale>
#include <KMessageBox>
#include <KDebug>

// LibQApt/DebconfKDE includes
#include <LibQApt/Backend>
#include <DebconfGui.h>

// Own includes
#include "../libmuon/ChangesDialog.h"
#include "Application.h"
#include "ReviewsBackend/ReviewsBackend.h"
#include "Transaction/Transaction.h"

ApplicationBackend::ApplicationBackend(QObject *parent)
    : QObject(parent)
    , m_backend(0)
    , m_reviewsBackend(new ReviewsBackend(this))
    , m_isReloading(false)
    , m_currentTransaction(0)
{
    m_pkgBlacklist << "kdebase-runtime" << "kdepim-runtime" << "kdelibs5-plugins" << "kdelibs5-data";
    
    connect(this, SIGNAL(reloadFinished()), SIGNAL(updatesCountChanged()));
}

ApplicationBackend::~ApplicationBackend()
{
}

void ApplicationBackend::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_backend->setUndoRedoCacheSize(1);
    m_reviewsBackend->setAptBackend(m_backend);
    init();

    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode,QVariantMap)),
            this, SLOT(errorOccurred(QApt::ErrorCode,QVariantMap)));

    emit appBackendReady();
}

void ApplicationBackend::init()
{
    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QStringList("*.desktop"), QDir::Files);

    QList<Application *> tempList;
    QSet<QString> packages;
    foreach(const QString &fileName, fileList) {
        Application *app = new Application(appDir.filePath(fileName), m_backend);
        packages.insert(app->packageName());
        tempList << app;
    }

    foreach (QApt::Package *package, m_backend->availablePackages()) {
        //Don't create applications twice
        if(packages.contains(package->name())) {
            continue;
        }
        Application *app = new Application(package, m_backend);
        tempList << app;
    }

    foreach (Application *app, tempList) {
        bool added = false;
        if (app->isValid()) {
            QApt::Package *pkg = app->package();
            if ((pkg) && !m_pkgBlacklist.contains(pkg->latin1Name())) {
                m_appList << app;
                if (pkg->isInstalled()) {
                    m_instOriginList << pkg->origin();
                }
                m_originList << pkg->origin();
                added = true;
            }
        }

        if(!added)
            delete app;
    }

    m_originList.remove(QString());
    m_instOriginList.remove(QString());
}

void ApplicationBackend::reload()
{
    emit reloadStarted();
    m_isReloading = true;
    foreach(Application* app, m_appList)
        app->clearPackage();
    qDeleteAll(m_queue);
    m_queue.clear();
    m_reviewsBackend->stopPendingJobs();
    m_backend->reloadCache();

    emit reloadFinished();
    m_isReloading = false;
}

bool ApplicationBackend::isReloading() const
{
    return m_isReloading;
}

void ApplicationBackend::workerEvent(QApt::WorkerEvent event)
{
    m_workerState.first = event;

    if (event == QApt::XapianUpdateFinished && !isReloading()) {
        emit xapianReloaded();
    }

    if (!m_queue.isEmpty()) {
        m_workerState.second = m_currentTransaction;
    } else {
        return;
    }

    emit workerEvent(event, m_currentTransaction);

    // Due to bad design on my part, we can get events from other apps.
    // This is required to ensure that we only handle events for stuff we started
    if (!m_currentTransaction) {
        return;
    }

    switch (event) {
    case QApt::PackageDownloadStarted:
        m_currentTransaction->setState(RunningState);
        connect(m_backend, SIGNAL(downloadProgress(int,int,int)),
                this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_backend, SIGNAL(downloadProgress(int,int,int)),
                   this, SLOT(updateDownloadProgress(int)));
        break;
    case QApt::CommitChangesStarted:
        m_debconfGui = new DebconfKde::DebconfGui("/tmp/qapt-sock");
        m_currentTransaction->setState(RunningState);
        connect(m_backend, SIGNAL(commitProgress(QString,int)),
                this, SLOT(updateCommitProgress(QString,int)));
        m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
        m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
        break;
    case QApt::CommitChangesFinished: {
        disconnect(m_backend, SIGNAL(commitProgress(QString,int)),
                   this, SLOT(updateCommitProgress(QString,int)));

        m_currentTransaction->setState(DoneState);

        Transaction* t = m_queue.dequeue();
        Q_ASSERT(t==m_currentTransaction);
        transactionRemoved(t);

        if (m_currentTransaction->action() == InstallApp) {
            m_appLaunchList << m_currentTransaction->application();
            emit launchListChanged();
        }

        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
        m_currentTransaction->application()->emitInstallChanged();
        delete m_currentTransaction;

        if (m_queue.isEmpty()) {
            reload();
        } else {
            runNextTransaction();
        }
    }   break;
    default:
        break;
    }
}

void ApplicationBackend::errorOccurred(QApt::ErrorCode error, const QVariantMap &details)
{
    Q_UNUSED(details);

    if (m_queue.isEmpty()) {
        emit errorSignal(error, details);
        return;
    }

    disconnect(m_backend, SIGNAL(downloadProgress(int,int,int)),
                   this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(QString,int)),
                   this, SLOT(updateCommitProgress(QString,int)));

    // Undo marking if an AuthError is encountered, since our install/remove
    // buttons do both marking and committing
    switch (error) {
    case QApt::UserCancelError:
        // Handled in transactionCancelled()
        return;
    default:
        cancelTransaction(m_currentTransaction->application());
        m_backend->undo();
        break;
    }

    // Reset worker state on failure
    if (m_workerState.second) {
        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
    }

    // A CommitChangesFinished signal will still be fired in this case,
    // and workerEvent will take care of queue management for us
    emit errorSignal(error, details);
}

void ApplicationBackend::updateDownloadProgress(int percentage)
{
    emit progress(m_currentTransaction, percentage);
}

void ApplicationBackend::updateCommitProgress(const QString &text, int percentage)
{
    Q_UNUSED(text);

    emit progress(m_currentTransaction, percentage);
}

bool ApplicationBackend::confirmRemoval(Transaction *transaction)
{
    QApt::CacheState oldCacheState = m_backend->currentCacheState();

    // Simulate transaction
    markTransaction(transaction);

    // Find changes due to markings
    QApt::PackageList excluded;
    excluded.append(transaction->application()->package());
    QApt::StateChanges changes = m_backend->stateChanges(oldCacheState, excluded);
    // Restore cache state, we're only checking at the moment
    m_backend->restoreCacheState(oldCacheState);

    if (changes[QApt::Package::ToRemove].isEmpty()) {
        return true;
    }
    QHash<QApt::Package::State, QApt::PackageList> removals;
    removals[QApt::Package::ToRemove] = changes[QApt::Package::ToRemove];

    ChangesDialog *dialog = new ChangesDialog(0, removals);

    return (dialog->exec() == QDialog::Accepted);
}

void ApplicationBackend::markTransaction(Transaction *transaction)
{
    Application *app = transaction->application();

    switch (transaction->action()) {
    case InstallApp:
        app->package()->setInstall();
        break;
    case RemoveApp:
        app->package()->setRemove();
        break;
    default:
        break;
    }

    QHash<QApt::Package *, QApt::Package::State> addons = transaction->addons();
    auto iter = addons.constBegin();

    QApt::Package *package;
    QApt::Package::State state;
    while (iter != addons.constEnd()) {
        package = iter.key();
        state = iter.value();
        switch (state) {
        case QApt::Package::ToInstall:
            package->setInstall();
            break;
        case QApt::Package::ToRemove:
            package->setRemove();
            break;
        default:
            break;
        }
        ++iter;
    }
}

void ApplicationBackend::addTransaction(Transaction *transaction)
{
    if (m_isReloading || !confirmRemoval(transaction)) {
        emit transactionCancelled(transaction->application());
        delete transaction;
        return;
    }

    transaction->setState(QueuedState);
    m_queue.enqueue(transaction);
    emit transactionAdded(transaction);

    if (m_queue.count() == 1) {
        m_currentTransaction = m_queue.head();
        runNextTransaction();
        emit startingFirstTransaction();
    }
}

void ApplicationBackend::cancelTransaction(Application *app)
{
    disconnect(m_backend, SIGNAL(downloadProgress(int,int,int)),
               this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(QString,int)),
               this, SLOT(updateCommitProgress(QString,int)));
    QQueue<Transaction *>::iterator iter = m_queue.begin();

    while (iter != m_queue.end()) {
        if ((*iter)->application() == app) {
            if ((*iter)->state() == RunningState) {
                m_backend->cancelDownload();
                m_backend->undo();
            }

            Transaction* t = *iter;
            transactionRemoved(t);
            m_queue.erase(iter);
            delete t;
            break;
        }
        ++iter;
    }

    emit transactionCancelled(app);
}

void ApplicationBackend::runNextTransaction()
{
    m_currentTransaction = m_queue.head();
    if (!m_currentTransaction) {
        return;
    }
    QApt::CacheState oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();

    Application *app = m_currentTransaction->application();

    markTransaction(m_currentTransaction);

    if (app->package()->wouldBreak()) {
        m_backend->restoreCacheState(oldCacheState);
        //TODO Notify of error
    }

    m_backend->commitChanges();
}

QList<Application*> ApplicationBackend::launchList() const
{
    return m_appLaunchList;
}

void ApplicationBackend::clearLaunchList()
{
    m_appLaunchList.clear();
}

ReviewsBackend *ApplicationBackend::reviewsBackend() const
{
    return m_reviewsBackend;
}

QList<Application *> ApplicationBackend::applicationList() const
{
    return m_appList;
}

QSet<QString> ApplicationBackend::appOrigins() const
{
    return m_originList;
}

QSet<QString> ApplicationBackend::installedAppOrigins() const
{
    return m_instOriginList;
}

QPair<QApt::WorkerEvent, Transaction *> ApplicationBackend::workerState() const
{
    return m_workerState;
}

QList<Transaction *> ApplicationBackend::transactions() const
{
    return m_queue;
}

void ApplicationBackend::installApplication(Application *app, const QHash<QApt::Package *, QApt::Package::State> &addons)
{
    TransactionAction action = InvalidAction;

    if (app->package()->isInstalled()) {
        action = ChangeAddons;
    } else {
        action = InstallApp;
    }

    Transaction *transaction = new Transaction(app, action, addons);
    addTransaction(transaction);
}

void ApplicationBackend::installApplication(Application *app)
{
    addTransaction(new Transaction(app, InstallApp));
}

void ApplicationBackend::removeApplication(Application *app)
{
    addTransaction(new Transaction(app, RemoveApp));
}

int ApplicationBackend::updatesCount() const
{
    if(m_isReloading)
        return 0;
    
    int count = 0;
    foreach(Application* app, m_appList) {
        count += app->canUpgrade();
    }
    return count;
}
