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
#include "../libmuonapt/HistoryView/HistoryView.h"
#include "../libmuonapt/MuonStrings.h"
#include "TransactionWidget.h"
#include "FilterWidget/FilterWidget.h"
#include "ManagerWidget.h"
#include "ReviewWidget.h"
#include "MuonSettings.h"
#include "StatusWidget.h"
#include "config/ManagerSettingsDialog.h"
#include "../libmuonapt/QAptActions.h"

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
    connect(m_managerWidget, SIGNAL(doneSortingPackages(bool)),
            this, SLOT(setCanExit(bool)));

    m_transWidget = new TransactionWidget(this);

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

    m_stack->addWidget(m_transWidget);
    m_stack->addWidget(m_mainWidget);
    m_stack->setCurrentWidget(m_mainWidget);

    m_backend = new QApt::Backend(this);
    QAptActions* actions = QAptActions::self();

    actions->setMainWindow(this);
    connect(actions, SIGNAL(changesReverted()), this, SLOT(revertChanges()));
    connect(actions, SIGNAL(checkForUpdates()), this, SLOT(checkForUpdates()));
    setupActions();

    m_statusWidget = new StatusWidget(centralWidget);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_statusWidget, SLOT(setBackend(QApt::Backend*)));
    centralLayout->addWidget(m_statusWidget);
}

void MainWindow::initObject()
{
    if (!m_backend->init())
        QAptActions::self()->initError();

    emit backendReady(m_backend);
    QAptActions::self()->setBackend(m_backend);
    connect(m_backend, SIGNAL(packageChanged()),
            this, SLOT(setActionsEnabled()));

    // Set up GUI
    loadSettings();
    setActionsEnabled();
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

    m_stack->setCurrentWidget(m_transWidget);
    m_trans = m_backend->updateCache();
    setupTransaction(m_trans);

    m_trans->run();
}

void MainWindow::errorOccurred(QApt::ErrorCode error)
{
    switch(error) {
    case QApt::AuthError:
    case QApt::LockError:
        m_managerWidget->setEnabled(true);
        QApplication::restoreOverrideCursor();
        returnFromPreview();
        setActionsEnabled();
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
        m_stack->setCurrentWidget(m_transWidget);
        break;
    case QApt::FinishedStatus:
        reload();
        setActionsEnabled();

        m_trans->deleteLater();
        m_trans = nullptr;
        break;
    default:
        break;
    }
}

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

    m_stack->setCurrentWidget(m_transWidget);
    m_trans = m_backend->commitChanges();
    setupTransaction(m_trans);

    m_trans->run();
}

void MainWindow::reload()
{
    setCanExit(false);
    returnFromPreview();
    m_stack->setCurrentWidget(m_mainWidget);

    // Reload the QApt Backend
    m_managerWidget->reload();

    // Reload other widgets
    if (m_reviewWidget) {
        m_reviewWidget->reload();
    }

    m_filterBox->reload();

    QAptActions::self()->setOriginalState(m_backend->currentCacheState());
    m_statusWidget->updateStatus();
    setActionsEnabled();
    m_managerWidget->setEnabled(true);

    setCanExit(true);
}

void MainWindow::setActionsEnabled(bool enabled)
{
    QAptActions::self()->setActionsEnabled(enabled);
    if (!enabled) {
        m_applyAction->setEnabled(false);
        m_safeUpgradeAction->setEnabled(false);
        m_distUpgradeAction->setEnabled(false);
        m_autoRemoveAction->setEnabled(false);
        m_previewAction->setEnabled(false);
        return;
    }

    int upgradeable = m_backend->packageCount(QApt::Package::Upgradeable);
    bool changesPending = m_backend->areChangesMarked();
    int autoRemoveable = m_backend->packageCount(QApt::Package::IsGarbage);

    m_applyAction->setEnabled(changesPending);
    m_safeUpgradeAction->setEnabled(upgradeable > 0);
    m_distUpgradeAction->setEnabled(upgradeable > 0);
    m_autoRemoveAction->setEnabled(autoRemoveable > 0);
    if (m_stack->currentWidget() == m_reviewWidget) {
        // We always need to be able to get back from review
        m_previewAction->setEnabled(true);
    } else {
        m_previewAction->setEnabled(changesPending);
    }
}

void MainWindow::downloadArchives(QApt::Transaction *trans)
{
    if (!trans) {
        // Shouldn't happen...
        delete trans;
        return;
    }

    m_stack->setCurrentWidget(m_transWidget);
    m_trans = trans;
    setupTransaction(trans);
    trans->run();
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

    trans->setDebconfPipe(m_transWidget->pipe());
    m_transWidget->setTransaction(m_trans);

    connect(m_trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(transactionStatusChanged(QApt::TransactionStatus)));
    connect(m_trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            this, SLOT(errorOccurred(QApt::ErrorCode)));
}
