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
#include <QtCore/QUuid>
#include <QTimer>

// KDE includes
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KProtocolManager>
#include <KStandardDirs>
#include <KIO/Job>
#include <KDebug>
#include <KAboutData>
#include <KAction>
#include <KActionCollection>

// LibQApt/DebconfKDE includes
#include <LibQApt/Backend>
#include <LibQApt/Transaction>
#include <DebconfGui.h>
#include <qjson/parser.h>

//libmuonapt includes
#include "MuonStrings.h"
#include "ChangesDialog.h"
#include "QAptActions.h"

// Own includes
#include "Application.h"
#include "ReviewsBackend.h"
#include "Transaction/Transaction.h"
#include "ApplicationUpdates.h"
#include "MuonMainWindow.h"
#include <MuonDataSources.h>

static const KCatalogLoader loader("app-install-data");

K_PLUGIN_FACTORY(MuonAppsBackendFactory, registerPlugin<ApplicationBackend>(); )
K_EXPORT_PLUGIN(MuonAppsBackendFactory(KAboutData("muon-appsbackend","muon-appsbackend",ki18n("Applications Backend"),"0.1",ki18n("Applications in your system"), KAboutData::License_GPL)))

ApplicationBackend::ApplicationBackend(QObject* parent, const QVariantList& )
    : AbstractResourcesBackend(parent)
    , m_backend(new QApt::Backend(this))
    , m_reviewsBackend(new ReviewsBackend(this))
    , m_isReloading(false)
    , m_currentTransaction(nullptr)
    , m_backendUpdater(new ApplicationUpdates(this))
    , m_aptify(nullptr)
    , m_aptBackendInitialized(false)
{
    KGlobal::dirs()->addResourceDir("appicon", "/usr/share/app-install/icons/");
    
    m_watcher = new QFutureWatcher<QVector<Application*> >(this);
    connect(m_watcher, SIGNAL(finished()), this, SLOT(setApplications()));
    connect(this, SIGNAL(reloadFinished()), SIGNAL(updatesCountChanged()));
    connect(this, SIGNAL(backendReady()), SIGNAL(updatesCountChanged()));
    connect(m_reviewsBackend, SIGNAL(ratingsReady()), SIGNAL(allDataChanged()));
    connect(m_backendUpdater, SIGNAL(updatesFinnished()), SLOT(reload()));
    
    QTimer::singleShot(10, this, SLOT(initBackend()));
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

    QList<Application *> tempList;
    QSet<QString> packages;
    foreach(const QString &fileName, fileList) {
        Application *app = new Application(appDir.filePath(fileName), backend);
        packages.insert(app->packageName());
        tempList << app;
    }

    foreach (QApt::Package *package, backend->availablePackages()) {
        //Don't create applications twice
        if(packages.contains(package->name()))
            continue;

        if (package->isMultiArchDuplicate())
            continue;

        Application *app = new Application(package, backend);
        tempList << app;
    }

    // To be added an Application must have a package that:
    // a) exists
    // b) is not on the blacklist
    // c) if not downloadable, then it must already be installed
    for (Application *app : tempList) {
        bool added = false;
        QApt::Package *pkg = app->package();
        if (app->isValid() && pkg) {
            appList << app;
            app->moveToThread(thread);
            added = true;
        }

        if(!added)
            delete app;
    }

    return appList;
}

void ApplicationBackend::setApplications()
{
    m_appList = m_watcher->future().result();
    for (Application* app : m_appList)
        app->setParent(this);

    emit backendReady();
    
    KIO::StoredTransferJob* job = KIO::storedGet(KUrl(MuonDataSources::screenshotsSource(), "/json/packages"),KIO::NoReload, KIO::DefaultFlags|KIO::HideProgressInfo);
    connect(job, SIGNAL(finished(KJob*)), SLOT(initAvailablePackages(KJob*)));

    if (m_aptify)
        m_aptify->setCanExit(true);
}

void ApplicationBackend::reload()
{
    if (m_aptify)
        m_aptify->setCanExit(false);
    emit reloadStarted();
    m_isReloading = true;
    foreach(Application* app, m_appList)
        app->clearPackage();
    qDeleteAll(m_transQueue);
    m_transQueue.clear();
    m_reviewsBackend->stopPendingJobs();

    if (!m_backend->reloadCache())
        QAptActions::self()->initError();

    foreach(Application* app, m_appList)
        app->package();

    m_isReloading = false;
    if (m_aptify)
        m_aptify->setCanExit(true);
    emit reloadFinished();
    emit searchInvalidated();
}

bool ApplicationBackend::isReloading() const
{
    return m_isReloading;
}

void ApplicationBackend::aptTransactionsChanged(QString active)
{
    // Find the newly-active QApt transaction in our list
    QApt::Transaction *trans = nullptr;
    auto list = m_transQueue.values();

    for (QApt::Transaction *t : list) {
        if (t->transactionId() == active) {
            trans = t;
            break;
        }
    }

    if (!trans || m_transQueue.key(trans) == m_currentTransaction)
        return;

    m_currentTransaction = m_transQueue.key(trans);
    connect(trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(transactionEvent(QApt::TransactionStatus)));
    connect(trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            this, SLOT(errorOccurred(QApt::ErrorCode)));
    connect(trans, SIGNAL(progressChanged(int)),
            this, SLOT(updateProgress(int)));
    // FIXME: untrusted packages, conf file prompt, media change
}

void ApplicationBackend::transactionEvent(QApt::TransactionStatus status)
{
    auto iter = m_transQueue.find(m_currentTransaction);
    if (iter == m_transQueue.end())
        return;

    switch (status) {
    case QApt::SetupStatus:
    case QApt::AuthenticationStatus:
    case QApt::WaitingStatus:
    case QApt::WaitingLockStatus:
    case QApt::WaitingMediumStatus:
    case QApt::WaitingConfigFilePromptStatus:
    case QApt::LoadingCacheStatus:
        break;
    case QApt::RunningStatus:
        m_currentTransaction->setState(RunningState);
        break;
    case QApt::DownloadingStatus:
        transactionsEvent(StartedDownloading, m_currentTransaction);
        break;
    case QApt::CommittingStatus:
        transactionsEvent(FinishedDownloading, m_currentTransaction);
        transactionsEvent(StartedCommitting, m_currentTransaction);

        // Set up debconf
        m_debconfGui = new DebconfKde::DebconfGui(iter.value()->debconfPipe());
        m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
        m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
        break;
    case QApt::FinishedStatus:
        transactionsEvent(FinishedCommitting, m_currentTransaction);
        m_currentTransaction->setState(DoneState);

        // Clean up manually created debconf pipe
        QApt::Transaction *trans = iter.value();
        if (!trans->debconfPipe().isEmpty())
            QFile::remove(trans->debconfPipe());

        // Cleanup
        trans->deleteLater();
        emit transactionRemoved(m_currentTransaction);
        m_transQueue.remove(iter.key());

        qobject_cast<Application*>(m_currentTransaction->resource())->emitStateChanged();
        delete m_currentTransaction;
        m_currentTransaction = nullptr;

        if (m_transQueue.isEmpty())
            reload();
        break;
    }
}

void ApplicationBackend::errorOccurred(QApt::ErrorCode error)
{
    if (m_transQueue.isEmpty()) // Shouldn't happen
        return;

    QAptActions::self()->displayTransactionError(error, m_transQueue.value(m_currentTransaction));
}

void ApplicationBackend::updateProgress(int percentage)
{
    emit transactionProgressed(m_currentTransaction, percentage);
}

bool ApplicationBackend::confirmRemoval(QApt::StateChanges changes)
{
    const QApt::PackageList removeList = changes.value(QApt::Package::ToRemove);
    if (removeList.isEmpty())
        return true;

    QApt::StateChanges removals;
    removals[QApt::Package::ToRemove] = removeList;

    QPointer<ChangesDialog> dialog = new ChangesDialog(mainWindow(), removals);
    bool ret = dialog->exec() == QDialog::Accepted;
    delete dialog;
    return ret;
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
    QApt::CacheState oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();

    markTransaction(transaction);

    // Find changes due to markings
    QApt::PackageList excluded;
    excluded.append(qobject_cast<Application*>(transaction->resource())->package());

    // Exclude addons being marked
    auto iter = transaction->addons().constBegin();
    while (iter != transaction->addons().constEnd()) {
        QApt::Package *addon = m_backend->package(iter.key());
        if (addon)
            excluded.append(addon);
        ++iter;
    }

    QApt::StateChanges changes = m_backend->stateChanges(oldCacheState, excluded);

    if (!confirmRemoval(changes)) {
        m_backend->restoreCacheState(oldCacheState);
        emit transactionCancelled(transaction);
        delete transaction;
        return;
    }

    Application *app = qobject_cast<Application*>(transaction->resource());

    if (app->package()->wouldBreak()) {
        m_backend->restoreCacheState(oldCacheState);
        //TODO Notify of error
    }

    QApt::Transaction *aptTrans = m_backend->commitChanges();
    setupTransaction(aptTrans);
    m_transQueue.insert(transaction, aptTrans);
    aptTrans->run();
    m_backend->restoreCacheState(oldCacheState); // Undo temporary simulation marking
    emit transactionAdded(transaction);

    if (m_transQueue.count() == 1) {
        aptTransactionsChanged(aptTrans->transactionId());
        m_currentTransaction = transaction;
        emit startingFirstTransaction();
    }
}

void ApplicationBackend::cancelTransaction(AbstractResource* app)
{
    for (auto iter = m_transQueue.begin(); iter != m_transQueue.end(); ++iter) {
        Transaction* t = iter.key();
        QApt::Transaction *aptTrans = iter.value();

        if (t->resource() == app) {
            if (t->state() == RunningState) {
                aptTrans->cancel();
            }
            break;
        }
    }
    // Emitting the cancellation occurs when the QApt trans is finished
}

//void ApplicationBackend::runNextTransaction()
//{
//    m_currentTransaction = m_queue.head();
//    if (!m_currentTransaction) {
//        return;
//    }
//    QApt::CacheState oldCacheState = m_backend->currentCacheState();
//    m_backend->saveCacheState();

//    markTransaction(m_currentTransaction);

//    Application *app = qobject_cast<Application*>(m_currentTransaction->resource());
    
//    if (app->package()->wouldBreak()) {
//        m_backend->restoreCacheState(oldCacheState);
//        //TODO Notify of error
//    }

//    m_backend->commitChanges();
//    m_backend->undo(); // Undo temporary simulation marking
//}

QApt::Backend* ApplicationBackend::backend() const
{
    return m_backend;
}

AbstractReviewsBackend *ApplicationBackend::reviewsBackend() const
{
    return m_reviewsBackend;
}

QVector<AbstractResource*> ApplicationBackend::allResources() const
{
    QVector<AbstractResource*> ret;

    for (Application* app : m_appList) {
        ret += app;
    }
    return ret;
}

QList<Transaction *> ApplicationBackend::transactions() const
{
    return m_transQueue.keys();
}

void ApplicationBackend::installApplication(AbstractResource* res, const QHash< QString, bool >& addons)
{
    Application* app = qobject_cast<Application*>(res);
    TransactionAction action = app->package()->isInstalled() ? ChangeAddons : InstallApp;

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
    QPair<TransactionStateTransition, Transaction*> ret;
    ret.second = m_currentTransaction;

    QApt::Transaction *trans = m_transQueue.value(m_currentTransaction);

    if (!m_currentTransaction || !trans)
        return ret;

    switch (trans->status()) {
    case QApt::DownloadingStatus:
        ret.first = StartedDownloading;
        break;
    case QApt::CommittingStatus:
        ret.first = StartedCommitting;
        break;
    default:
        break;
    }

    return ret;
}

AbstractBackendUpdater* ApplicationBackend::backendUpdater() const
{
    return m_backendUpdater;
}

void ApplicationBackend::integrateMainWindow(MuonMainWindow* w)
{
    m_aptify = w;
    QAptActions* apt = QAptActions::self();
    apt->setMainWindow(w);
    if(m_aptBackendInitialized)
        apt->setBackend(m_backend);
    else
        connect(this, SIGNAL(aptBackendInitialized(QApt::Backend*)), apt, SLOT(setBackend(QApt::Backend*)));
    connect(apt, SIGNAL(sourcesEditorClosed(bool)), SLOT(reload()));

    KAction* updateAction = w->actionCollection()->addAction("update");
    updateAction->setIcon(KIcon("system-software-update"));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    updateAction->setEnabled(w->isConnected());
    connect(updateAction, SIGNAL(triggered()), SLOT(checkForUpdates()));
    connect(w, SIGNAL(shouldConnect(bool)), updateAction, SLOT(setEnabled(bool)));
}

QWidget* ApplicationBackend::mainWindow() const
{
    return QAptActions::self()->mainWindow();
}

void ApplicationBackend::initBackend()
{
    if (m_aptify) {
        m_aptify->setCanExit(false);
        QAptActions::self()->setReloadWhenEditorFinished(true);
    }

    if (!m_backend->init())
        QAptActions::self()->initError();
    if (m_backend->xapianIndexNeedsUpdate()) {
        // FIXME: transaction
        m_backend->updateXapianIndex();
    }
    m_aptBackendInitialized = true;
    emit aptBackendInitialized(m_backend);

    m_backend->setUndoRedoCacheSize(1);
    m_reviewsBackend->setAptBackend(m_backend);
    m_backendUpdater->setBackend(m_backend);

    QFuture<QVector<Application*> > future = QtConcurrent::run(init, m_backend, QThread::currentThread());
    m_watcher->setFuture(future);
    connect(m_backend, SIGNAL(transactionQueueChanged(QString,QStringList)),
            this, SLOT(aptTransactionsChanged(QString)));
    connect(m_backend, SIGNAL(xapianUpdateFinished()),
            this, SIGNAL(searchInvalidated()));
    if(m_aptify)
        m_aptify->setCanExit(true);
}

void ApplicationBackend::setupTransaction(QApt::Transaction *trans)
{
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor("http"));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, 0)));

    // Debconf
    QString uuid = QUuid::createUuid().toString();
    uuid.remove('{').remove('}').remove('-');
    QFile pipe(QDir::tempPath() % QLatin1String("/qapt-sock-") % uuid);
    pipe.open(QFile::ReadWrite);
    pipe.close();
    trans->setDebconfPipe(pipe.fileName());
}

void ApplicationBackend::sourcesEditorClosed()
{
    reload();
    emit sourcesEditorFinished();
}

void ApplicationBackend::initAvailablePackages(KJob* j)
{
    KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);
    Q_ASSERT(job);
    
    bool ok=false;
    QJson::Parser p;
    QVariantList data = p.parse(job->data(), &ok).toMap().value("packages").toList();
    if(!ok)
        kWarning() << "errors!" << p.errorString();
    else {
        Q_ASSERT(!m_appList.isEmpty());
        QSet<QString> packages;
        foreach(const QVariant& v, data) {
            packages += v.toMap().value("name").toString();
        }
        Q_ASSERT(packages.count()==data.count());
        for(Application* a : m_appList) {
            a->setHasScreenshot(packages.contains(a->packageName()));
        }
    }
}

QList< AbstractResource* > ApplicationBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_appList) {
        if(r->state()==AbstractResource::Upgradeable)
            ret+=r;
    }
    return ret;
}

void ApplicationBackend::checkForUpdates()
{
    QAptActions::self()->setActionsEnabled(false);
    QApt::Transaction* transaction = backend()->updateCache();
    m_backendUpdater->setupTransaction(transaction);
    connect(transaction, SIGNAL(finished(QApt::ExitStatus)), SLOT(reload()));
    connect(transaction, SIGNAL(finished(QApt::ExitStatus)), transaction, SLOT(deleteLater()));
    transaction->run();

}
