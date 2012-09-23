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

#include "MainWindow.h"

// Qt includes
#include <QApplication>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBox>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KFileDialog>
#include <KLocale>
#include <KMessageBox>
#include <KProtocolManager>
#include <KStandardAction>
#include <KStatusBar>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/Config>
#include <LibQApt/Transaction>

// Own includes
#include "../libmuon/HistoryView/HistoryView.h"
#include "TransactionWidget.h"
#include "FilterWidget/FilterWidget.h"
#include "ManagerWidget.h"
#include "ReviewWidget.h"
#include "MuonSettings.h"
#include "StatusWidget.h"
#include "config/ManagerSettingsDialog.h"
#include "../libmuon/QAptActions.h"

MainWindow::MainWindow()
    : MuonMainWindow()
    , m_settingsDialog(nullptr)
    , m_historyDialog(nullptr)
    , m_reviewWidget(nullptr)
    , m_transWidget(nullptr)

{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

MainWindow::~MainWindow()
{
    MuonSettings::self()->writeConfig();
}

void MainWindow::initGUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setSpacing(0);
    centralLayout->setMargin(0);

    m_stack = new QStackedWidget(centralWidget);
    m_stack->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    centralLayout->addWidget(m_stack);

    setCentralWidget(centralWidget);

    m_managerWidget = new ManagerWidget(m_stack);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_managerWidget, SLOT(setBackend(QApt::Backend*)));
    connect(m_managerWidget, SIGNAL(packageChanged()), this, SLOT(setActionsEnabled()));

    m_mainWidget = new QSplitter(this);
    m_mainWidget->setOrientation(Qt::Horizontal);
    connect(m_mainWidget, SIGNAL(splitterMoved(int,int)), this, SLOT(saveSplitterSizes()));

    m_filterBox = new FilterWidget(m_stack);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_filterBox, SLOT(setBackend(QApt::Backend*)));
    connect(m_filterBox, SIGNAL(filterByGroup(QString)),
            m_managerWidget, SLOT(filterByGroup(QString)));
    connect(m_filterBox, SIGNAL(filterByStatus(QApt::Package::State)),
            m_managerWidget, SLOT(filterByStatus(QApt::Package::State)));
    connect(m_filterBox, SIGNAL(filterByOrigin(QString)),
            m_managerWidget, SLOT(filterByOrigin(QString)));
    connect(m_filterBox, SIGNAL(filterByArchitecture(QString)),
            m_managerWidget, SLOT(filterByArchitecture(QString)));

    m_mainWidget->addWidget(m_filterBox);
    m_mainWidget->addWidget(m_managerWidget);
    loadSplitterSizes();

    m_stack->addWidget(m_mainWidget);
    m_stack->setCurrentWidget(m_mainWidget);

    m_backend = new QApt::Backend(this);

    m_actions = new QAptActions(this, m_backend);
    connect(m_actions, SIGNAL(changesReverted()),
            this, SLOT(revertChanges()));
    connect(m_actions, SIGNAL(checkForUpdates()),
            this, SLOT(checkForUpdates()));
    setupActions();

    m_statusWidget = new StatusWidget(centralWidget);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_statusWidget, SLOT(setBackend(QApt::Backend*)));
    centralLayout->addWidget(m_statusWidget);
}

void MainWindow::initObject()
{
    m_backend->init();
    m_canExit = true;

    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    emit backendReady(m_backend);

    // Set up GUI
    loadSettings();
    m_actions->setActionsEnabled();
    m_managerWidget->setFocus();
}

void MainWindow::loadSettings()
{
    m_backend->setUndoRedoCacheSize(MuonSettings::self()->undoStackSize());
    m_managerWidget->invalidateFilter();
}

void MainWindow::loadSplitterSizes()
{
    QList<int> sizes = MuonSettings::self()->splitterSizes();

    if (sizes.isEmpty()) {
        sizes << 115 << (this->width() - 115);
    }
    m_mainWidget->setSizes(sizes);
}

void MainWindow::saveSplitterSizes()
{
    MuonSettings::self()->setSplitterSizes(m_mainWidget->sizes());
    MuonSettings::self()->writeConfig();
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();
    m_actions->setupActions();

    m_loadSelectionsAction = actionCollection()->addAction("open_markings");
    m_loadSelectionsAction->setIcon(KIcon("document-open"));
    m_loadSelectionsAction->setText(i18nc("@action", "Read Markings..."));
    connect(m_loadSelectionsAction, SIGNAL(triggered()), m_actions, SLOT(loadSelections()));

    m_saveSelectionsAction = actionCollection()->addAction("save_markings");
    m_saveSelectionsAction->setIcon(KIcon("document-save-as"));
    m_saveSelectionsAction->setText(i18nc("@action", "Save Markings As..."));
    connect(m_saveSelectionsAction, SIGNAL(triggered()), m_actions, SLOT(saveSelections()));

    m_createDownloadListAction = actionCollection()->addAction("save_download_list");
    m_createDownloadListAction->setIcon(KIcon("document-save-as"));
    m_createDownloadListAction->setText(i18nc("@action", "Save Package Download List..."));
    connect(m_createDownloadListAction, SIGNAL(triggered()), m_actions, SLOT(createDownloadList()));

    m_downloadListAction = actionCollection()->addAction("download_from_list");
    m_downloadListAction->setIcon(KIcon("download"));
    m_downloadListAction->setText(i18nc("@action", "Download Packages From List..."));
    connect(m_downloadListAction, SIGNAL(triggered()), m_actions, SLOT(downloadPackagesFromList()));
    if (!m_actions->isConnected()) {
        m_downloadListAction->setDisabled(false);
    }
    connect(m_actions, SIGNAL(shouldConnect(bool)), m_downloadListAction, SLOT(setEnabled(bool)));

    m_loadArchivesAction = actionCollection()->addAction("load_archives");
    m_loadArchivesAction->setIcon(KIcon("document-open"));
    m_loadArchivesAction->setText(i18nc("@action", "Add Downloaded Packages"));
    connect(m_loadArchivesAction, SIGNAL(triggered()), m_actions, SLOT(loadArchives()));

    m_saveInstalledAction = actionCollection()->addAction("save_package_list");
    m_saveInstalledAction->setIcon(KIcon("document-save-as"));
    m_saveInstalledAction->setText(i18nc("@action", "Save Installed Packages List..."));
    connect(m_saveInstalledAction, SIGNAL(triggered()), m_actions, SLOT(saveInstalledPackagesList()));

    m_safeUpgradeAction = actionCollection()->addAction("safeupgrade");
    m_safeUpgradeAction->setIcon(KIcon("go-up"));
    m_safeUpgradeAction->setText(i18nc("@action Marks upgradeable packages for upgrade", "Cautious Upgrade"));
    connect(m_safeUpgradeAction, SIGNAL(triggered()), this, SLOT(markUpgrade()));

    m_distUpgradeAction = actionCollection()->addAction("fullupgrade");
    m_distUpgradeAction->setIcon(KIcon("go-top"));
    m_distUpgradeAction->setText(i18nc("@action Marks upgradeable packages, including ones that install/remove new things",
                                       "Full Upgrade"));
    connect(m_distUpgradeAction, SIGNAL(triggered()), this, SLOT(markDistUpgrade()));

    m_autoRemoveAction = actionCollection()->addAction("autoremove");
    m_autoRemoveAction->setIcon(KIcon("trash-empty"));
    m_autoRemoveAction->setText(i18nc("@action Marks packages no longer needed for removal",
                                      "Remove Unnecessary Packages"));
    connect(m_autoRemoveAction, SIGNAL(triggered()), this, SLOT(markAutoRemove()));

    m_previewAction = actionCollection()->addAction("preview");
    m_previewAction->setIcon(KIcon("document-preview-archive"));
    m_previewAction->setText(i18nc("@action Takes the user to the preview page", "Preview Changes"));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Applys the changes a user has made", "Apply Changes"));
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    m_historyAction = actionCollection()->addAction("history");
    m_historyAction->setIcon(KIcon("view-history"));
    m_historyAction->setText(i18nc("@action::inmenu", "History..."));
    m_historyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    connect(m_historyAction, SIGNAL(triggered()), this, SLOT(showHistoryDialog()));

    setActionsEnabled(false);

    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::markUpgrade()
{
    m_backend->saveCacheState();
    m_backend->markPackagesForUpgrade();

    if (m_backend-> markedPackages().isEmpty()) {
        QString text = i18nc("@label", "Unable to mark upgrades. The "
                             "available upgrades may require new packages to "
                             "be installed or removed. You may wish to try "
                             "a full upgrade by clicking the <interface>Full "
                             " Upgrade</interface> button.");
        QString title = i18nc("@title:window", "Unable to Mark Upgrades");
        KMessageBox::information(this, text, title);
    } else {
        previewChanges();
    }
}

void MainWindow::markDistUpgrade()
{
    m_backend->saveCacheState();
    m_backend->markPackagesForDistUpgrade();
    if (m_backend-> markedPackages().isEmpty()) {
        QString text = i18nc("@label", "Unable to mark upgrades. Some "
                             "upgrades may have unsatisfiable dependencies at "
                             "the moment, or may have been manually held back.");
        QString title = i18nc("@title:window", "Unable to Mark Upgrades");
        KMessageBox::information(this, text, title);
    } else {
        previewChanges();
    }
}

void MainWindow::markAutoRemove()
{
    m_backend->saveCacheState();
    m_backend->markPackagesForAutoRemove();
    previewChanges();
}

void MainWindow::checkForUpdates()
{
    setActionsEnabled(false);
    m_managerWidget->setEnabled(false);

    initTransactionWidget();
    m_trans = m_backend->updateCache();
    setupTransaction(m_trans);

    m_trans->run();
}

void MainWindow::downloadPackagesFromList()
{
    // FIXME: transactify
    initTransactionWidget();
    //MuonMainWindow::downloadPackagesFromList();
}

void MainWindow::errorOccurred(QApt::ErrorCode error)
{
    // FIXME: transactify
    //MuonMainWindow::errorOccurred(error);

    switch(error) {
    // FIXME: react to user cancel
//    case QApt::UserCancelError:
//        if (m_downloadWidget) {
//            m_downloadWidget->clear();
//        }
    case QApt::AuthError:
    case QApt::LockError:
        m_managerWidget->setEnabled(true);
        QApplication::restoreOverrideCursor();
        returnFromPreview();
        break;
    default:
        break;
    }
}

void MainWindow::transactionStatusChanged(QApt::TransactionStatus status)
{
    // FIXME: better support for transactions that do/don't need reloads
    switch (status) {
    case QApt::RunningStatus:
    case QApt::WaitingStatus:
        QApplication::restoreOverrideCursor();
        if (m_trans->role() != QApt::UpdateXapianRole)
            m_stack->setCurrentWidget(m_transWidget);
        break;
    case QApt::FinishedStatus:
        if (m_trans->role() != QApt::UpdateXapianRole) {
            reload();
            setActionsEnabled();
        }

        m_trans->deleteLater();
        m_trans = nullptr;
        break;
    default:
        break;
    }
}

//void MainWindow::workerEvent(QApt::WorkerEvent event)
//{
//    switch (event) {
//    case QApt::CacheUpdateStarted:
//        if (m_downloadWidget) {
//            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Updating software sources</title>"));
//            m_stack->setCurrentWidget(m_downloadWidget);
//            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
//        }
//        break;
//    case QApt::CacheUpdateFinished:
//    case QApt::CommitChangesFinished:
//        if (m_backend) {
//            reload();
//            setActionsEnabled();
//        }
//    case QApt::PackageDownloadFinished:
//        returnFromPreview();

//        if (m_downloadWidget) {
//            m_downloadWidget->deleteLater();
//            m_downloadWidget = nullptr;
//        }
//        break;
//    case QApt::PackageDownloadStarted:
//        if (m_downloadWidget) {
//            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Downloading Packages</title>"));
//            m_stack->setCurrentWidget(m_downloadWidget);
//            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
//        }
//        QApplication::restoreOverrideCursor();
//        break;
//    case QApt::CommitChangesStarted:
//        if (m_commitWidget) {
//            m_commitWidget->setHeaderText(i18nc("@info", "<title>Committing Changes</title>"));
//            m_stack->setCurrentWidget(m_commitWidget);
//        }
//        QApplication::restoreOverrideCursor();
//        break;
//    case QApt::XapianUpdateStarted:
//        m_statusWidget->showXapianProgress();
//        connect(m_backend, SIGNAL(xapianUpdateProgress(int)),
//                m_statusWidget, SLOT(updateXapianProgress(int)));
//        break;
//    case QApt::XapianUpdateFinished:
//        m_managerWidget->startSearch();
//        disconnect(m_backend, SIGNAL(xapianUpdateProgress(int)),
//                   m_statusWidget, SLOT(updateXapianProgress(int)));
//        m_statusWidget->hideXapianProgress();
//        break;
//    case QApt::InvalidEvent:
//    default:
//        break;
//    }
//}

void MainWindow::previewChanges()
{
    m_reviewWidget = new ReviewWidget(m_stack);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_reviewWidget, SLOT(setBackend(QApt::Backend*)));
    m_reviewWidget->setBackend(m_backend);
    m_stack->addWidget(m_reviewWidget);

    m_stack->setCurrentWidget(m_reviewWidget);

    m_previewAction->setIcon(KIcon("go-previous"));
    m_previewAction->setText(i18nc("@action:intoolbar Return from the preview page", "Back"));
    disconnect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(returnFromPreview()));
}

void MainWindow::returnFromPreview()
{
    m_stack->setCurrentWidget(m_mainWidget);
    if (m_reviewWidget) {
        m_reviewWidget->deleteLater();
        m_reviewWidget = nullptr;
    }

    m_previewAction->setIcon(KIcon("document-preview-archive"));
    m_previewAction->setText(i18nc("@action", "Preview Changes"));
    disconnect(m_previewAction, SIGNAL(triggered()), this, SLOT(returnFromPreview()));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));
}

void MainWindow::startCommit()
{
    setActionsEnabled(false);
    m_managerWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    initTransactionWidget();
    m_trans = m_backend->commitChanges();
    setupTransaction(m_trans);

    m_trans->run();
}

void MainWindow::initTransactionWidget()
{
    if (!m_transWidget) {
        m_transWidget = new TransactionWidget(this);
        m_stack->addWidget(m_transWidget);
    }
}

void MainWindow::reload()
{
    m_canExit = false;
    returnFromPreview();
    m_stack->setCurrentWidget(m_mainWidget);

    // No need to keep these around in memory.
    if (m_transWidget) {
        m_transWidget->deleteLater();
        m_transWidget = nullptr;
    }

    // Reload the QApt Backend
    m_managerWidget->reload();

    // Maybe reload the xapian index
    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    // Reload other widgets
    if (m_reviewWidget) {
        m_reviewWidget->reload();
    }

    m_filterBox->reload();

    m_actions->setOriginalState(m_backend->currentCacheState());
    m_statusWidget->updateStatus();
    setActionsEnabled();
    m_managerWidget->setEnabled(true);

    m_canExit = true;
}

void MainWindow::setActionsEnabled(bool enabled)
{
    m_actions->setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    int upgradeable = m_backend->packageCount(QApt::Package::Upgradeable);
    bool changesPending = m_backend->areChangesMarked();
    int autoRemoveable = m_backend->packageCount(QApt::Package::IsGarbage);

    m_safeUpgradeAction->setEnabled(upgradeable > 0);
    m_distUpgradeAction->setEnabled(upgradeable > 0);
    m_autoRemoveAction->setEnabled(autoRemoveable > 0);
    if (m_stack->currentWidget() == m_reviewWidget) {
        // We always need to be able to get back from review
        m_previewAction->setEnabled(true);
    } else {
        m_previewAction->setEnabled(changesPending);
    }

    m_downloadListAction->setEnabled(m_actions->isConnected());

    m_applyAction->setEnabled(changesPending);

    m_loadSelectionsAction->setEnabled(true);
    m_saveSelectionsAction->setEnabled(changesPending);
    m_saveInstalledAction->setEnabled(true);
}

void MainWindow::editSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new ManagerSettingsDialog(this, m_backend->config());
        connect(m_settingsDialog, SIGNAL(finished()), SLOT(closeSettingsDialog()));
        connect(m_settingsDialog, SIGNAL(settingsChanged()), SLOT(loadSettings()));
        m_settingsDialog->show();
    } else {
        m_settingsDialog->raise();
    }
}

void MainWindow::closeSettingsDialog()
{
    m_settingsDialog->deleteLater();
    m_settingsDialog = nullptr;
}

void MainWindow::showHistoryDialog()
{
    if (!m_historyDialog) {
        m_historyDialog = new KDialog(this);

        KConfigGroup dialogConfig(KSharedConfig::openConfig("muonrc"),
                                  "HistoryDialog");
        m_historyDialog->restoreDialogSize(dialogConfig);

        connect(m_historyDialog, SIGNAL(finished()), SLOT(closeHistoryDialog()));
        HistoryView *historyView = new HistoryView(m_historyDialog);
        m_historyDialog->setMainWidget(historyView);
        m_historyDialog->setWindowTitle(i18nc("@title:window", "Package History"));
        m_historyDialog->setWindowIcon(KIcon("view-history"));
        m_historyDialog->setButtons(KDialog::Close);
        m_historyDialog->show();
    } else {
        m_historyDialog->raise();
    }
}

void MainWindow::closeHistoryDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("muonrc"),
                              "HistoryDialog");
    m_historyDialog->saveDialogSize(dialogConfig, KConfigBase::Persistent);
    m_historyDialog->deleteLater();
    m_historyDialog = nullptr;
}

void MainWindow::revertChanges()
{
    if (m_reviewWidget) {
        returnFromPreview();
    }
}

void MainWindow::setupTransaction(QApt::Transaction *trans)
{
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor("http"));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, 0)));

    QString pipe = QLatin1String("/tmp/") + QUuid::createUuid().toString();
    pipe.remove('{').remove('}').remove('-');

    trans->setDebconfPipe(pipe);
    m_transWidget->setTransaction(m_trans);

    connect(m_trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(transactionStatusChanged(QApt::TransactionStatus)));
}
