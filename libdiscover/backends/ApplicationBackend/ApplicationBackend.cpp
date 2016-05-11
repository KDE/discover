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
#include <QSignalMapper>
#include <QJsonDocument>
#include <QAction>

// KDE includes
#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KProtocolManager>
#include <KIO/Job>
#include <KActionCollection>
#include <KIconLoader>
#include <KXmlGuiWindow>

#include <AppstreamQt/database.h>

// QApt/DebconfKDE includes
#include <QApt/Backend>
#include <QApt/Transaction>
#include <DebconfGui.h>

//libmuonapt includes
#include "MuonStrings.h"
#include "ChangesDialog.h"
#include "QAptActions.h"

// Own includes
#include "AptSourcesBackend.h"
#include "Application.h"
#include "ReviewsBackend.h"
#include "Transaction/Transaction.h"
#include "Transaction/TransactionModel.h"
#include "ApplicationUpdates.h"
#include <resources/SourcesModel.h>
#include <MuonDataSources.h>

MUON_BACKEND_PLUGIN(ApplicationBackend)

class AptTransaction : public Transaction
{
public:
    AptTransaction(QObject *parent, AbstractResource *resource, Transaction::Role role, const AddonList &addons = {})
        : Transaction(parent, resource, role, addons), m_aptTrans(nullptr) {}

    void cancel() {
        m_aptTrans->cancel();
    }

    QApt::Transaction *aptTrans() const { return m_aptTrans; }
    void setAptTrans(QApt::Transaction *t) { m_aptTrans = t; }

private:
    QApt::Transaction *m_aptTrans;
};

ApplicationBackend::ApplicationBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_backend(new QApt::Backend(this))
    , m_reviewsBackend(new ReviewsBackend(this))
    , m_isFetching(true)
    , m_currentTransaction(nullptr)
    , m_backendUpdater(new ApplicationUpdates(this))
    , m_aptify(nullptr)
    , m_aptBackendInitialized(false)
{
    KIconLoader::global()->reconfigure(QString(), QStringList(QStringLiteral("/usr/share/app-install/icons/")));

    m_watcher = new QFutureWatcher<QVector<Application*> >(this);
    connect(m_watcher, &QFutureWatcher<QVector<Application*> >::finished, this, &ApplicationBackend::setApplications);
    connect(m_reviewsBackend, &ReviewsBackend::ratingsReady, this, &ApplicationBackend::allDataChanged);
    
    QTimer::singleShot(10, this, SLOT(initBackend()));
}

ApplicationBackend::~ApplicationBackend()
{
    qDeleteAll(m_appList);
}

QVector<Application *> init(QApt::Backend *backend, QThread* thread)
{
    Appstream::Database appdata;
    bool opened = appdata.open();
    Q_ASSERT(opened);

    QVector<Application *> tempList;
    QSet<QString> packages;
    foreach(const Appstream::Component &component, appdata.allComponents()) {
        if (component.packageNames().isEmpty())
            continue;
        Application *app = new Application(component, backend);
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
    QVector<Application *> appList;
    appList.reserve(tempList.size());
    Q_FOREACH (Application *app, tempList) {
        QApt::Package *pkg = app->package();
        if (app->isValid() && pkg)
        {
            appList << app;
            app->moveToThread(thread);
        }
        else
            delete app;
    }

    return appList;
}

void ApplicationBackend::setApplications()
{
    m_appList = m_watcher->future().result();
    Q_FOREACH (Application* app, m_appList)
        app->setParent(this);

    KIO::StoredTransferJob* job = KIO::storedGet(QUrl(MuonDataSources::screenshotsSource().toString() + QStringLiteral("/json/packages")),KIO::NoReload, KIO::DefaultFlags|KIO::HideProgressInfo);
    connect(job, &KIO::StoredTransferJob::finished, this, &ApplicationBackend::initAvailablePackages);

    if (m_aptify)
        QAptActions::self()->setCanExit(true);
    setFetching(false);
}

void ApplicationBackend::reload()
{
    if(isFetching()) {
        qWarning() << "Reloading while already reloading... Please report.";
        return;
    }
    setFetching(true);
    if (m_aptify)
        QAptActions::self()->setCanExit(false);
    foreach(Application* app, m_appList)
        app->clearPackage();
    qDeleteAll(m_transQueue);
    m_transQueue.clear();
    m_reviewsBackend->stopPendingJobs();

    if (!m_backend->reloadCache())
        QAptActions::self()->initError();

    foreach(Application* app, m_appList)
        app->package();

    if (m_aptify)
        QAptActions::self()->setCanExit(true);
    setFetching(false);
}

bool ApplicationBackend::isFetching() const
{
    return m_isFetching;
}

bool ApplicationBackend::isValid() const
{
    // ApplicationBackend will force an application quit if it is invalid, so
    // if it has not done that, the backend is valid.
    return true;
}

void ApplicationBackend::aptTransactionsChanged(QString active)
{
    // Find the newly-active QApt transaction in our list
    AptTransaction* discTrans = nullptr;

    Q_FOREACH (AptTransaction *t, m_transQueue) {
        if (t->aptTrans()->transactionId() == active) {
            discTrans = t;
            break;
        }
    }

    if (!discTrans || discTrans == m_currentTransaction)
        return;

    m_currentTransaction = discTrans;
    QApt::Transaction *trans = discTrans->aptTrans();
    connect(trans, &QApt::Transaction::statusChanged, this, &ApplicationBackend::transactionEvent);
    connect(trans, &QApt::Transaction::errorOccurred, this, &ApplicationBackend::errorOccurred);
    connect(trans, &QApt::Transaction::progressChanged, this, &ApplicationBackend::updateProgress);
    // FIXME: untrusted packages, conf file prompt, media change
}

void ApplicationBackend::transactionEvent(QApt::TransactionStatus status)
{
    if (!m_currentTransaction->aptTrans())
        return;

    TransactionModel *transModel = TransactionModel::global();

    switch (status) {
    case QApt::SetupStatus:
    case QApt::AuthenticationStatus:
    case QApt::WaitingStatus:
    case QApt::WaitingLockStatus:
    case QApt::WaitingMediumStatus:
    case QApt::WaitingConfigFilePromptStatus:
    case QApt::LoadingCacheStatus:
        m_currentTransaction->setStatus(Transaction::SetupStatus);
        break;
    case QApt::RunningStatus:
        m_currentTransaction->setStatus(Transaction::QueuedStatus);
        break;
    case QApt::DownloadingStatus:
        m_currentTransaction->setStatus(Transaction::DownloadingStatus);
        m_currentTransaction->setCancellable(false);
        break;
    case QApt::CommittingStatus:
        m_currentTransaction->setStatus(Transaction::CommittingStatus);

        // Set up debconf
        m_debconfGui = new DebconfKde::DebconfGui(m_currentTransaction->aptTrans()->debconfPipe());
        m_debconfGui->connect(m_debconfGui, &DebconfKde::DebconfGui::activated, m_debconfGui, &DebconfKde::DebconfGui::show);
        m_debconfGui->connect(m_debconfGui, &DebconfKde::DebconfGui::deactivated, m_debconfGui, &DebconfKde::DebconfGui::hide);
        break;
    case QApt::FinishedStatus:
        m_currentTransaction->setStatus(Transaction::DoneStatus);

        // Clean up manually created debconf pipe
        QApt::Transaction *trans = m_currentTransaction->aptTrans();
        if (!trans->debconfPipe().isEmpty())
            QFile::remove(trans->debconfPipe());

        // Cleanup
        trans->deleteLater();

        if (trans->exitStatus() == QApt::ExitCancelled)
            transModel->cancelTransaction(m_currentTransaction);
        else
            transModel->removeTransaction(m_currentTransaction);
        m_transQueue.removeAll(m_currentTransaction);

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

    if( error == QApt::AuthError){
        m_currentTransaction->cancel();
        m_transQueue.removeAll(m_currentTransaction);
        m_currentTransaction->deleteLater();
        m_currentTransaction = nullptr;
    }
    QAptActions::self()->displayTransactionError(error, m_currentTransaction->aptTrans());
}

void ApplicationBackend::updateProgress(int percentage)
{
    if(!m_currentTransaction) {
        qDebug() << "missing transaction";
        return;
    }
    m_currentTransaction->setProgress(percentage);
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

    switch (transaction->role()) {
    case Transaction::InstallRole:
        app->package()->setInstall();
        markLangpacks(transaction);
        break;
    case Transaction::RemoveRole:
        app->package()->setRemove();
        break;
    default:
        break;
    }

    AddonList addons = transaction->addons();

    Q_FOREACH (const QString &pkgStr, addons.addonsToInstall()) {
        QApt::Package *package = m_backend->package(pkgStr);
        package->setInstall();
    }

    Q_FOREACH (const QString &pkgStr, addons.addonsToRemove()) {
        QApt::Package *package = m_backend->package(pkgStr);
        package->setRemove();
    }
}

void ApplicationBackend::markLangpacks(Transaction *transaction)
{
    QString prog = QStandardPaths::findExecutable(QStringLiteral("check-language-support"));
    if (prog.isEmpty()){
        prog = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("discover/scripts/check-language-support"));
        if ( prog.isEmpty()){
            return;
        }
    }

    QString language = QLocale().name();
    QString pkgName = transaction->resource()->packageName();

    QStringList args;
    args << prog << QLatin1String("-l") << language << QLatin1String("-p") << pkgName;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::OnlyStdoutChannel);
    proc.setProgram(args);
    proc.start();
    proc.waitForFinished();

    QString res = QString::fromLatin1(proc.readAllStandardOutput());
    res.remove(QString());

    m_backend->setCompressEvents(true);
    foreach(const QString &pkg, res.split(QLatin1Char(' ')))
    {
        QApt::Package *langPack = m_backend->package(pkg.trimmed());

        if (langPack)
            langPack->setInstall();
    }
    m_backend->setCompressEvents(false);
}

void ApplicationBackend::addTransaction(AptTransaction *transaction)
{
    if(!transaction){
      return;
    }
    QApt::CacheState oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();

    markTransaction(transaction);

    // Find changes due to markings
    QApt::PackageList excluded;
    excluded.append(qobject_cast<Application*>(transaction->resource())->package());
    // Exclude addons being marked
    Q_FOREACH (const QString &pkgStr, transaction->addons().addonsToInstall()) {
        QApt::Package *addon = m_backend->package(pkgStr);

        if (addon)
            excluded.append(addon);
    }

    Q_FOREACH (const QString &pkgStr, transaction->addons().addonsToRemove()) {
        QApt::Package *addon = m_backend->package(pkgStr);

        if (addon)
            excluded.append(addon);
    }

    QApt::StateChanges changes = m_backend->stateChanges(oldCacheState, excluded);

    if (!confirmRemoval(changes)) {
        m_backend->restoreCacheState(oldCacheState);
        transaction->deleteLater();
        return;
    }

    Application *app = qobject_cast<Application*>(transaction->resource());

    if (app->package()->wouldBreak()) {
        m_backend->restoreCacheState(oldCacheState);
        //TODO Notify of error
    }

    QApt::Transaction *aptTrans = m_backend->commitChanges();
    setupTransaction(aptTrans);
    transaction->setAptTrans(aptTrans);
    TransactionModel::global()->addTransaction(transaction);
    aptTrans->run();
    m_backend->restoreCacheState(oldCacheState); // Undo temporary simulation marking

    if (m_transQueue.count() == 1) {
        aptTransactionsChanged(aptTrans->transactionId());
        m_currentTransaction = transaction;
    }
}

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

    Q_FOREACH (Application* app, m_appList) {
        ret += app;
    }
    return ret;
}

void ApplicationBackend::installApplication(AbstractResource* res, const AddonList& addons)
{
    Application* app = qobject_cast<Application*>(res);
    Transaction::Role role = app->package()->isInstalled() ? Transaction::ChangeAddonsRole : Transaction::InstallRole;
    addTransaction(new AptTransaction(this, res, role, addons));
}

void ApplicationBackend::installApplication(AbstractResource* app)
{
    addTransaction(new AptTransaction(this, app, Transaction::InstallRole));
}

void ApplicationBackend::removeApplication(AbstractResource* app)
{
    addTransaction(new AptTransaction(this, app, Transaction::RemoveRole));
}

int ApplicationBackend::updatesCount() const
{
    if(m_isFetching)
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
    return nullptr;
}

QList<AbstractResource*> ApplicationBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> resources;
    if(m_isFetching) {
        qWarning() << "searching while fetching!!!";
        return resources;
    }

    QSet<QApt::Package*> packages = m_backend->search(searchText).toSet();

    foreach(Application* a, m_appList) {
        if(packages.contains(a->package())) {
            resources += a;
        }
    }
    return resources;
}

AbstractBackendUpdater* ApplicationBackend::backendUpdater() const
{
    return m_backendUpdater;
}

void ApplicationBackend::integrateActions(KActionCollection* w)
{
    m_aptify = w;
    QAptActions* apt = QAptActions::self();
    apt->setActionCollection(w);
    if(!m_aptBackendInitialized)
        connect(this, &ApplicationBackend::aptBackendInitialized, apt, &QAptActions::setBackend);
    if (apt->reloadWhenSourcesEditorFinished())
        connect(apt, &QAptActions::sourcesEditorClosed, this, &ApplicationBackend::reload);
    QAction* updateAction = w->addAction(QStringLiteral("update"));
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    updateAction->setEnabled(apt->isConnected());
    connect(updateAction, &QAction::triggered, this, &ApplicationBackend::checkForUpdates);
    connect(apt, &QAptActions::shouldConnect, updateAction, &QAction::setEnabled);
}

QWidget* ApplicationBackend::mainWindow() const
{
    return QAptActions::self()->mainWindow();
}

void ApplicationBackend::initBackend()
{
    setFetching(true);
    if (m_aptify) {
        QAptActions::self()->setCanExit(false);
        QAptActions::self()->setReloadWhenEditorFinished(true);
    }

    QAptActions::self()->setBackend(m_backend);
    if (m_backend->xapianIndexNeedsUpdate())
        m_backend->updateXapianIndex();

    m_aptBackendInitialized = true;
    emit aptBackendInitialized(m_backend);

    m_backend->setUndoRedoCacheSize(1);
    m_reviewsBackend->setAptBackend(m_backend);
    m_backendUpdater->setBackend(m_backend);

    QFuture<QVector<Application*> > future = QtConcurrent::run(init, m_backend, QThread::currentThread());
    m_watcher->setFuture(future);
    connect(m_backend, &QApt::Backend::transactionQueueChanged, this, &ApplicationBackend::aptTransactionsChanged);
    connect(m_backend, &QApt::Backend::xapianUpdateFinished, this, &ApplicationBackend::searchInvalidated);

    SourcesModel::global()->addSourcesBackend(new AptSourcesBackend(this));
}

void ApplicationBackend::setupTransaction(QApt::Transaction *trans)
{
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor(QStringLiteral("http")));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, nullptr)));

    // Debconf
    QString uuid = QUuid::createUuid().toString();
    uuid.remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));
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

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(job->data(), &error);
    if(error.error != QJsonParseError::NoError)
        qWarning() << "errors!" << error.errorString();
    else {
        QVariantList data = doc.toVariant().toMap().value(QStringLiteral("packages")).toList();
        Q_ASSERT(!m_appList.isEmpty());
        QSet<QString> packages;
        foreach(const QVariant& v, data) {
            packages += v.toMap().value(QStringLiteral("name")).toString();
        }
        Q_ASSERT(packages.count()==data.count());
        Q_FOREACH (Application* a, m_appList) {
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
    QApt::Transaction* transaction = backend()->updateCache();
    m_backendUpdater->setupTransaction(transaction);
    transaction->run();
    m_backendUpdater->setProgressing(true);
    connect(transaction, &QApt::Transaction::finished, this, &ApplicationBackend::updateFinished);
}

void ApplicationBackend::updateFinished(QApt::ExitStatus status)
{
    if(status != QApt::ExitSuccess) {
        qWarning() << "updating was not successful";
    }
    m_backendUpdater->setProgressing(false);
}

void ApplicationBackend::setFetching(bool f)
{
    if(m_isFetching != f) {
        m_isFetching = f;
        emit fetchingChanged();
        if(!m_isFetching) {
            emit searchInvalidated();
            emit updatesCountChanged();
        }
    }
}

QList<QAction*> ApplicationBackend::messageActions() const
{
    QList<QAction*> ret;
    //high priority
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("dist-upgrade"));

    //normal priority
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("update"));

    //low priority
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("software_properties"));
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("load_archives"));
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("save_package_list"));
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("download_from_list"));
    ret += QAptActions::self()->actionCollection()->action(QStringLiteral("history"));
    Q_ASSERT(!ret.contains(nullptr));
    return ret;
}

#include "ApplicationBackend.moc"
