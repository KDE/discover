/***************************************************************************
 *   Copyright © 2010-2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "UpdaterWindow.h"

// Qt includes
#include <QApplication>
#include <QtCore/QTimer>
#include <QtGui/QStackedWidget>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KIcon>
#include <KLocale>
#include <KStatusBar>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "config/UpdaterSettingsDialog.h"
#include "../libmuon/CommitWidget.h"
#include "../libmuon/DownloadWidget.h"
#include "../libmuon/StatusWidget.h"
#include "UpdaterWidget.h"

UpdaterWindow::UpdaterWindow()
    : MuonMainWindow()
    , m_stack(0)
    , m_settingsDialog(0)
    , m_downloadWidget(0)
    , m_commitWidget(0)
{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

UpdaterWindow::~UpdaterWindow()
{
}

void UpdaterWindow::initGUI()
{
    setWindowTitle(i18nc("@title:window", "Software Updates"));
    m_stack = new QStackedWidget;
    setCentralWidget(m_stack);

    m_updaterWidget = new UpdaterWidget(m_stack);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_updaterWidget, SLOT(setBackend(QApt::Backend *)));

    m_stack->addWidget(m_updaterWidget);
    m_stack->setCurrentWidget(m_updaterWidget);

    setupActions();

    m_statusWidget = new StatusWidget(this);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_statusWidget, SLOT(setBackend(QApt::Backend *)));
    statusBar()->addWidget(m_statusWidget);
    statusBar()->show();
}

void UpdaterWindow::initObject()
{
    MuonMainWindow::initObject();

    setActionsEnabled(); //Get initial enabled/disabled state
}

void UpdaterWindow::setupActions()
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
    m_applyAction->setEnabled(isConnected());
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));
    connect(this, SIGNAL(shouldConnect(bool)), m_applyAction, SLOT(setEnabled(bool)));

    m_revertAction = actionCollection()->addAction("revert");
    m_revertAction->setIcon(KIcon("document-revert"));
    m_revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));

    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    setActionsEnabled(false);

    setupGUI();
}

void UpdaterWindow::checkForUpdates()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    initDownloadWidget();
    m_backend->updateCache();
}

void UpdaterWindow::downloadPackagesFromList()
{
    initDownloadWidget();
    MuonMainWindow::downloadPackagesFromList();
}

void UpdaterWindow::workerEvent(QApt::WorkerEvent event)
{
    MuonMainWindow::workerEvent(event);

    switch (event) {
    case QApt::CacheUpdateStarted:
        if (m_downloadWidget) {
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Updating software sources</title>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        }
        break;
    case QApt::CacheUpdateFinished:
    case QApt::CommitChangesFinished:
        reload();
    case QApt::PackageDownloadFinished:
        returnFromPreview();
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
        if (m_downloadWidget) {
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Downloading Updates</title>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        }
        QApplication::restoreOverrideCursor();
        break;
    case QApt::CommitChangesStarted:
        if (m_commitWidget) {
            m_commitWidget->setHeaderText(i18nc("@info", "<title>Installing Updates</title>"));
            m_stack->setCurrentWidget(m_commitWidget);
        }
        QApplication::restoreOverrideCursor();
        break;
    case QApt::InvalidEvent:
    default:
        break;
    }
}

void UpdaterWindow::errorOccurred(QApt::ErrorCode error, const QVariantMap &args)
{
    MuonMainWindow::errorOccurred(error, args);

    switch(error) {
    case QApt::UserCancelError:
        m_updaterWidget->setEnabled(true);
        QApplication::restoreOverrideCursor();
        returnFromPreview();
        break;
    default:
        break;
    }
}

void UpdaterWindow::initDownloadWidget()
{
    if (!m_downloadWidget) {
        m_downloadWidget = new DownloadWidget(this);
        m_stack->addWidget(m_downloadWidget);
        connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                m_downloadWidget, SLOT(updateDownloadProgress(int, int, int)));
        connect(m_backend, SIGNAL(packageDownloadProgress(const QString &, int, const QString &, double, int)),
                m_downloadWidget, SLOT(updatePackageDownloadProgress(const QString &, int, const QString &, double, int)));
    }
}

void UpdaterWindow::initCommitWidget()
{
    if (!m_commitWidget) {
        m_commitWidget = new CommitWidget(this);
        m_stack->addWidget(m_commitWidget);
        connect(m_backend, SIGNAL(commitProgress(const QString &, int)),
                m_commitWidget, SLOT(updateCommitProgress(const QString &, int)));
    }
}

void UpdaterWindow::startCommit()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    initDownloadWidget();
    initCommitWidget();
    m_backend->commitChanges();
}

void UpdaterWindow::reload()
{
    returnFromPreview();
    m_updaterWidget->reload();
    m_statusWidget->updateStatus();
    setActionsEnabled();
    m_updaterWidget->setEnabled(true);

    // No need to keep these around in memory.
    if (m_downloadWidget) {
        m_downloadWidget->deleteLater();
        m_downloadWidget = 0;
    }
    if (m_commitWidget) {
        m_commitWidget->deleteLater();
        m_commitWidget = 0;
    }

    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }
}

void UpdaterWindow::returnFromPreview()
{
    m_stack->setCurrentWidget(m_updaterWidget);
    m_backend->markPackagesForDistUpgrade();

    // We may not have anything to preview; check.
    setActionsEnabled();
}

void UpdaterWindow::setActionsEnabled(bool enabled)
{
    MuonMainWindow::setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    if (isConnected()) {
        m_applyAction->setEnabled(m_backend->areChangesMarked());
        m_downloadListAction->setEnabled(true);
    } else {
        m_applyAction->setEnabled(false);
        m_downloadListAction->setEnabled(false);
    }

    m_undoAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_redoAction->setEnabled(!m_backend->isRedoStackEmpty());
    m_revertAction->setEnabled(!m_backend->isUndoStackEmpty());
}

void UpdaterWindow::editSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new UpdaterSettingsDialog(this);
        connect(m_settingsDialog, SIGNAL(okClicked()), SLOT(closeSettingsDialog()));
        m_settingsDialog->show();
    } else {
        m_settingsDialog->raise();
    }
}

void UpdaterWindow::closeSettingsDialog()
{
    m_settingsDialog->deleteLater();
    m_settingsDialog = 0;
}

#include "UpdaterWindow.moc"
