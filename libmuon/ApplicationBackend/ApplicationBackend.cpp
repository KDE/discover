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
#include <QtConcurrentRun>
#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QtCore/QStringList>
#include <QTimer>

// KDE includes
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KStandardDirs>
#include <KDebug>

// LibQApt/DebconfKDE includes
#include <LibQApt/Backend>
#include <DebconfGui.h>

// Own includes
#include "ChangesDialog.h"
#include "Application.h"
#include "ReviewsBackend/ReviewsBackend.h"
#include "Transaction/Transaction.h"
#include "ApplicationUpdates.h"
#include "QAptIntegration.h"

ApplicationBackend::ApplicationBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_backend(0)
    , m_reviewsBackend(new ReviewsBackend(this))
    , m_isReloading(false)
    , m_currentTransaction(0)
    , m_backendUpdater(new ApplicationUpdates(this))
{
    m_watcher = new QFutureWatcher<QVector<Application*> >(this);
    connect(m_watcher, SIGNAL(finished()), this, SLOT(setApplications()));
    connect(this, SIGNAL(reloadFinished()), SIGNAL(updatesCountChanged()));
    connect(this, SIGNAL(backendReady()), SIGNAL(updatesCountChanged()));
    connect(m_reviewsBackend, SIGNAL(ratingsReady()), SIGNAL(allDataChanged()));
}

ApplicationBackend::~ApplicationBackend()
{
    qDeleteAll(m_appList);
}

QVector<Application *> init(QApt::Backend *backend, QThread* thread)
{
    QVector<Application *> appList;
    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QStringList("*.desktop"), QDir::Files);

    QStringList pkgBlacklist;
    pkgBlacklist << "kde-runtime" << "kdepim-runtime" << "kdelibs5-plugins" << "kdelibs5-data";

    QList<Application *> tempList;
    QSet<QString> packages;
    foreach(const QString &fileName, fileList) {
        Application *app = new Application(appDir.filePath(fileName), backend);
        packages.insert(app->packageName());
        tempList << app;
    }

    foreach (QApt::Package *package, backend->availablePackages()) {
        //Don't create applications twice
        if(packages.contains(package->name())) {
            continue;
        }

        if (package->isMultiArchDuplicate())
            continue;

        Application *app = new Application(package, backend);
        tempList << app;
    }

    foreach (Application *app, tempList) {
        bool added = false;
        QApt::Package *pkg = app->package();
        if (app->isValid()) {
            if ((pkg) && !pkgBlacklist.contains(pkg->latin1Name())) {
                appList << app;
                added = true;
            }
        }

        if(added)
            app->moveToThread(thread);
        else
            delete app;
    }

    return appList;
}

void ApplicationBackend::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_backend->setUndoRedoCacheSize(1);
    m_reviewsBackend->setAptBackend(m_backend);
    m_backendUpdater->setBackend(m_backend);

    QFuture<QVector<Application*> > future = QtConcurrent::run(init, m_backend, QThread::currentThread());
    m_watcher->setFuture(future);

    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode,QVariantMap)),
            this, SLOT(errorOccurred(QApt::ErrorCode,QVariantMap)));
}

void ApplicationBackend::setApplications()
{
    m_appList = m_watcher->future().result();

    // Populate origin lists
    QApt::Package *pkg;
    for (Application *app : m_appList) {
        app->setParent(this);
        pkg = app->package();
        if (pkg->isInstalled())
            m_instOriginList << pkg->origin();
        else
            m_originList << pkg->origin();
    }

    m_originList.remove(QString());
    m_instOriginList.remove(QString());
    m_originList += m_instOriginList;
    emit backendReady();
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

    foreach(Application* app, m_appList)
        app->package();

    m_isReloading = false;
    emit reloadFinished();
    emit searchInvalidated();
    emit updatesCountChanged();
}

bool ApplicationBackend::isReloading() const
{
    return m_isReloading;
}

void ApplicationBackend::workerEvent(QApt::WorkerEvent event)
{
    m_workerState.first = event;

    if (event == QApt::XapianUpdateFinished && !isReloading()) {
        emit searchInvalidated();
    }

    if (!m_queue.isEmpty()) {
        m_workerState.second = m_currentTransaction;
    } else {
        return;
    }

    // Due to bad design on my part, we can get events from other apps.
    // This is required to ensure that we only handle events for stuff we started
    if (!m_currentTransaction) {
        return;
    }

    emit workerEvent(event, m_currentTransaction);
    switch(event) {
        case QApt::PackageDownloadStarted: transactionsEvent(StartedDownloading, m_currentTransaction); break;
        case QApt::PackageDownloadFinished: transactionsEvent(FinishedDownloading, m_currentTransaction); break;
        case QApt::CommitChangesStarted: transactionsEvent(StartedCommitting, m_currentTransaction); break;
        case QApt::CommitChangesFinished: transactionsEvent(FinishedCommitting, m_currentTransaction); break;
        default: break;
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
        emit transactionRemoved(t);

        if (m_currentTransaction->action() == InstallApp) {
            m_appLaunchList << qobject_cast<Application*>(m_currentTransaction->resource());
            emit launchListChanged();
        }

        m_workerState.first = QApt::InvalidEvent;
        m_workerState.second = 0;
        qobject_cast<Application*>(m_currentTransaction->resource())->emitStateChanged();
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
        cancelTransaction(m_currentTransaction->resource());
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
    emit transactionProgressed(m_currentTransaction, percentage);
}

void ApplicationBackend::updateCommitProgress(const QString &text, int percentage)
{
    Q_UNUSED(text);

    emit transactionProgressed(m_currentTransaction, percentage);
}

bool ApplicationBackend::confirmRemoval(Transaction *transaction)
{
    QApt::CacheState oldCacheState = m_backend->currentCacheState();

    // Simulate transaction
    markTransaction(transaction);

    // Find changes due to markings
    QApt::PackageList excluded;
    excluded.append(qobject_cast<Application*>(transaction->resource())->package());
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
    Application *app = qobject_cast<Application*>(transaction->resource());

    switch (transaction->action()) {
    case InstallApp:
        app->package()->setInstall();
        markLangpacks(transaction);
        break;
    case RemoveApp:
        app->package()->setRemove();
        break;
    default:
        break;
    }

    QHash< QString, bool > addons = transaction->addons();
    auto iter = addons.constBegin();

    while (iter != addons.constEnd()) {
        QApt::Package* package = m_backend->package(iter.key());
        bool state = iter.value();
        if(state)
            package->setInstall();
        else
            package->setRemove();
        ++iter;
    }
}

void ApplicationBackend::markLangpacks(Transaction *transaction)
{
    QString prog = KStandardDirs::findExe("check-language-support");
    if (prog.isEmpty())
        return;

    QString language = KGlobal::locale()->language();
    QString pkgName = transaction->resource()->packageName();

    QStringList args;
    args << prog << QLatin1String("-l") << language << QLatin1String("-p") << pkgName;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::OnlyStdoutChannel);
    proc.setProgram(args);
    proc.start();
    proc.waitForFinished();

    QString res = proc.readAllStandardOutput();
    res.remove(QString());

    QApt::Package *langPack = nullptr;
    m_backend->setCompressEvents(true);
    foreach(const QString &pkg, res.split(' '))
    {
        langPack = m_backend->package(pkg.trimmed());

        if (langPack)
            langPack->setInstall();
    }
    m_backend->setCompressEvents(false);
}

void ApplicationBackend::addTransaction(Transaction *transaction)
{
    if (m_isReloading || !confirmRemoval(transaction)) {
        emit transactionCancelled(transaction);
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

void ApplicationBackend::cancelTransaction(AbstractResource* app)
{
    disconnect(m_backend, SIGNAL(downloadProgress(int,int,int)),
               this, SLOT(updateDownloadProgress(int)));
    disconnect(m_backend, SIGNAL(commitProgress(QString,int)),
               this, SLOT(updateCommitProgress(QString,int)));
    QQueue<Transaction *>::iterator iter = m_queue.begin();

    for (; iter != m_queue.end(); ++iter) {
        Transaction* t = *iter;
        if (t->resource() == app) {
            if (t->state() == RunningState) {
                m_backend->cancelDownload();
                m_backend->undo();
            }

            m_queue.erase(iter);
            emit transactionRemoved(t);
            emit transactionCancelled(t);
            delete t;
            break;
        }
    }
}

void ApplicationBackend::runNextTransaction()
{
    m_currentTransaction = m_queue.head();
    if (!m_currentTransaction) {
        return;
    }
    QApt::CacheState oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();

    markTransaction(m_currentTransaction);

    Application *app = qobject_cast<Application*>(m_currentTransaction->resource());
    
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

QApt::Backend* ApplicationBackend::backend() const
{
    return m_backend;
}

void ApplicationBackend::clearLaunchList()
{
    m_appLaunchList.clear();
}

AbstractReviewsBackend *ApplicationBackend::reviewsBackend() const
{
    return m_reviewsBackend;
}

QVector<Application *> ApplicationBackend::applicationList() const
{
    return m_appList;
}

QVector<AbstractResource*> ApplicationBackend::allResources() const
{
    QVector<AbstractResource*> ret;
    foreach(Application* app, m_appList) {
        ret += app;
    }
    return ret;
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

void ApplicationBackend::installApplication(AbstractResource* res, const QHash< QString, bool >& addons)
{
    Application* app = qobject_cast<Application*>(res);
    TransactionAction action = InvalidAction;

    if (app->package()->isInstalled()) {
        action = ChangeAddons;
    } else {
        action = InstallApp;
    }

    addTransaction(new Transaction(res, action, addons));
}

void ApplicationBackend::installApplication(AbstractResource* app)
{
    addTransaction(new Transaction(app, InstallApp));
}

void ApplicationBackend::removeApplication(AbstractResource* app)
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

AbstractResource* ApplicationBackend::resourceByPackageName(const QString& name) const
{
    foreach(Application* app, m_appList) {
        if(app->packageName()==name)
            return app;
    }
    return 0;
}

QStringList ApplicationBackend::searchPackageName(const QString& searchText)
{
    QApt::PackageList packages = m_backend->search(searchText);
    QStringList names;
    foreach(QApt::Package* package, packages) {
        names += package->name();
    }
    return names;
}

QPair<TransactionStateTransition, Transaction*> ApplicationBackend::currentTransactionState() const
{
    QPair< QApt::WorkerEvent, Transaction* > state = workerState();
    QPair<TransactionStateTransition, Transaction*> ret;
    switch(state.first) {
        case QApt::PackageDownloadStarted: ret.first = StartedDownloading; break;
        case QApt::PackageDownloadFinished: ret.first = FinishedDownloading; break;
        case QApt::CommitChangesStarted: ret.first = StartedCommitting; break;
        case QApt::CommitChangesFinished: ret.first = FinishedCommitting; break;
        default: break;
    }
    ret.second = state.second;
    return ret;
}

bool ApplicationBackend::providesResouce(AbstractResource* res) const
{
    return qobject_cast<Application*>(res);
}

AbstractBackendUpdater* ApplicationBackend::backendUpdater() const
{
    return m_backendUpdater;
}

void ApplicationBackend::integrateMainWindow(KXmlGuiWindow* w)
{
    QAptIntegration* aptify = new QAptIntegration(w);
    QTimer::singleShot(10, aptify, SLOT(initObject()));
    aptify->setupActions();
    connect(aptify, SIGNAL(backendReady(QApt::Backend*)), this, SLOT(setBackend(QApt::Backend*)));
}
