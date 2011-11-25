/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include "kmessagewidget.h"
#include <Solid/Device>
#include <Solid/AcAdapter>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/HistoryView/HistoryView.h"
#include "ChangelogWidget.h"
#include "ProgressWidget.h"
#include "config/UpdaterSettingsDialog.h"
#include "UpdaterWidget.h"

MainWindow::MainWindow()
    : MuonMainWindow()
    , m_settingsDialog(0)
    , m_historyDialog(0)
{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

void MainWindow::initGUI()
{
    setWindowTitle(i18nc("@title:window", "Software Updates"));

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    m_powerMessage = new KMessageWidget(mainWidget);
    m_powerMessage->setText(i18nc("@info Warning to plug in laptop before updating",
                                  "It is safer to plug in the power adapter before updating."));
    m_powerMessage->hide();
    m_powerMessage->setMessageType(KMessageWidget::Warning);
    checkPlugState();

    m_progressWidget = new ProgressWidget(mainWidget);
    m_progressWidget->hide();

    m_updaterWidget = new UpdaterWidget(mainWidget);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_updaterWidget, SLOT(setBackend(QApt::Backend *)));

    m_changelogWidget = new ChangelogWidget(this);
    m_changelogWidget->hide();
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_changelogWidget, SLOT(setBackend(QApt::Backend *)));
    connect(m_updaterWidget, SIGNAL(packageChanged(QApt::Package*)),
            m_changelogWidget, SLOT(setPackage(QApt::Package*)));

    mainLayout->addWidget(m_powerMessage);
    mainLayout->addWidget(m_progressWidget);
    mainLayout->addWidget(m_updaterWidget);
    mainLayout->addWidget(m_changelogWidget);

    setupActions();

    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);
}

void MainWindow::initObject()
{
    MuonMainWindow::initObject();
    setActionsEnabled(); //Get initial enabled/disabled state

    connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
            m_progressWidget, SLOT(updateDownloadProgress(int, int, int)));
    connect(m_backend, SIGNAL(commitProgress(const QString &, int)),
            m_progressWidget, SLOT(updateCommitProgress(const QString &, int)));
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    m_createDownloadListAction = actionCollection()->addAction("save_download_list");
    m_createDownloadListAction->setIcon(KIcon("document-save-as"));
    m_createDownloadListAction->setText(i18nc("@action", "Save Package Download List..."));
    connect(m_createDownloadListAction, SIGNAL(triggered()), this, SLOT(createDownloadList()));

    m_downloadListAction = actionCollection()->addAction("download_from_list");
    m_downloadListAction->setIcon(KIcon("download"));
    m_downloadListAction->setText(i18nc("@action", "Download Packages From List..."));
    connect(m_downloadListAction, SIGNAL(triggered()), this, SLOT(downloadPackagesFromList()));
    if (!isConnected()) {
        m_downloadListAction->setDisabled(false);
    }
    connect(this, SIGNAL(shouldConnect(bool)), m_downloadListAction, SLOT(setEnabled(bool)));

    m_loadArchivesAction = actionCollection()->addAction("load_archives");
    m_loadArchivesAction->setIcon(KIcon("document-open"));
    m_loadArchivesAction->setText(i18nc("@action", "Add Downloaded Packages"));
    connect(m_loadArchivesAction, SIGNAL(triggered()), this, SLOT(loadArchives()));

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Downloads and installs updates", "Install Updates"));
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

void MainWindow::workerEvent(QApt::WorkerEvent event)
{
    MuonMainWindow::workerEvent(event);

    switch (event) {
    case QApt::CacheUpdateStarted:
        m_progressWidget->show();
        m_progressWidget->setHeaderText(i18nc("@info", "<title>Updating software sources</title>"));
        connect(m_progressWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        break;
    case QApt::CacheUpdateFinished:
    case QApt::CommitChangesFinished:
        if (m_backend) {
            m_progressWidget->animatedHide();
            m_updaterWidget->setEnabled(true);
            m_updaterWidget->setCurrentIndex(0);
            reload();
            setActionsEnabled();
        }
    case QApt::PackageDownloadFinished:
        if (m_warningStack.size() > 0) {
            showQueuedWarnings();
            m_warningStack.clear();
        }
        if (m_errorStack.size() > 0) {
            showQueuedErrors();
            m_errorStack.clear();
        }
        break;
    case QApt::PackageDownloadStarted:
        m_progressWidget->show();
        m_progressWidget->setHeaderText(i18nc("@info", "<title>Downloading Updates</title>"));
        connect(m_progressWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        QApplication::restoreOverrideCursor();
        break;
    case QApt::CommitChangesStarted:
        m_progressWidget->setHeaderText(i18nc("@info", "<title>Installing Updates</title>"));
        QApplication::restoreOverrideCursor();
        break;
    default:
        break;
    }
}

void MainWindow::errorOccurred(QApt::ErrorCode error, const QVariantMap &args)
{
    MuonMainWindow::errorOccurred(error, args);

    switch (error) {
    case QApt::UserCancelError:
        if (m_backend) {
            m_progressWidget->animatedHide();
            m_updaterWidget->setEnabled(true);
            setActionsEnabled();
        }
        QApplication::restoreOverrideCursor();
        break;
    case QApt::AuthError:
        m_updaterWidget->setEnabled(true);
        setActionsEnabled();
        QApplication::restoreOverrideCursor();
    default:
        break;
    }
}

void MainWindow::reload()
{
    m_canExit = false;

    disconnect(m_updaterWidget, SIGNAL(packageChanged(QApt::Package*)),
               m_changelogWidget, SLOT(setPackage(QApt::Package*)));
    m_updaterWidget->reload();
    connect(m_updaterWidget, SIGNAL(packageChanged(QApt::Package*)),
            m_changelogWidget, SLOT(setPackage(QApt::Package*)));
    m_changelogWidget->setPackage(0);
    QApplication::restoreOverrideCursor();

    checkPlugState();

    m_canExit = true;
}

void MainWindow::setActionsEnabled(bool enabled)
{
    MuonMainWindow::setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    m_downloadListAction->setEnabled(isConnected());

    m_applyAction->setEnabled(m_backend->areChangesMarked());
    m_undoAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_redoAction->setEnabled(!m_backend->isRedoStackEmpty());
    m_revertAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_updaterWidget->setEnabled(true);
}

void MainWindow::checkForUpdates()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->animatedHide();
    m_changelogWidget->stopPendingJobs();
    m_backend->updateCache();
}

void MainWindow::startCommit()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->animatedHide();
    m_changelogWidget->stopPendingJobs();
    m_backend->commitChanges();
}

void MainWindow::editSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new UpdaterSettingsDialog(this);
        connect(m_settingsDialog, SIGNAL(okClicked()), SLOT(closeSettingsDialog()));
        m_settingsDialog->show();
    } else {
        m_settingsDialog->raise();
    }
}

void MainWindow::closeSettingsDialog()
{
    m_settingsDialog->deleteLater();
    m_settingsDialog = 0;
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
    m_historyDialog = 0;
}

void MainWindow::checkPlugState()
{
    const QList<Solid::Device> acAdapters = Solid::Device::listFromType(Solid::DeviceInterface::AcAdapter);

    if (acAdapters.isEmpty()) {
        updatePlugState(true);
        return;
    }
    
    bool isPlugged = false;

    foreach(Solid::Device device_ac, acAdapters) {
        Solid::AcAdapter* acAdapter = device_ac.as<Solid::AcAdapter>();
        isPlugged |= acAdapter->isPlugged();
        connect(acAdapter, SIGNAL(plugStateChanged(bool,QString)),
                this, SLOT(updatePlugState(bool)), Qt::UniqueConnection);
    }

    updatePlugState(isPlugged);
}

void MainWindow::updatePlugState(bool plugged)
{
    plugged ? m_powerMessage->hide() : m_powerMessage->show();
}
