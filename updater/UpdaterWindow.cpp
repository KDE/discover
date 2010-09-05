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

#include "UpdaterWindow.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QStackedWidget>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KIcon>
#include <KLocale>
#include <KStatusBar>
#include <Solid/PowerManagement>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "../libmuon/CommitWidget.h"
#include "../libmuon/DownloadWidget.h"
#include "../libmuon/StatusWidget.h"
#include "UpdaterWidget.h"

UpdaterWindow::UpdaterWindow()
    : MuonMainWindow()
    , m_stack(0)
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
    setWindowTitle(i18n("Software Updates"));
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
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(reloadActions()));

    reloadActions(); //Get initial enabled/disabled state
}

void UpdaterWindow::setupActions()
{
    MuonMainWindow::setupActions();

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Downloads and installs updates", "Install Updates"));
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    m_revertAction = actionCollection()->addAction("revert");
    m_revertAction->setIcon(KIcon("document-revert"));
    m_revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));

    setupGUI();
}

void UpdaterWindow::checkForUpdates()
{
    setActionsEnabled(false);
    initDownloadWidget();
    m_backend->updateCache();
}

void UpdaterWindow::workerEvent(QApt::WorkerEvent event)
{
    switch (event) {
    case QApt::CacheUpdateStarted:
        if (m_downloadWidget) {
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Updating software sources</title>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            m_powerInhibitor = Solid::PowerManagement::beginSuppressingSleep(i18nc("@info:status", "Muon is downloading packages"));
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        }
        break;
    case QApt::CacheUpdateFinished:
    case QApt::CommitChangesFinished:
        Solid::PowerManagement::stopSuppressingSleep(m_powerInhibitor);
        m_canExit = true;
        reload();
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
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Downloading Packages</title>"));
            m_powerInhibitor = Solid::PowerManagement::beginSuppressingSleep(i18nc("@info:status", "Muon is downloading packages"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
        }
        break;
    case QApt::CommitChangesStarted:
        if (m_commitWidget) {
            m_canExit = false;
            m_commitWidget->setHeaderText(i18nc("@info", "<title>Committing Changes</title>"));
            m_stack->setCurrentWidget(m_commitWidget);
        }
        break;
    case QApt::XapianUpdateStarted:
        break;
    case QApt::XapianUpdateFinished:
        m_backend->openXapianIndex();
        break;
    case QApt::PackageDownloadFinished:
    case QApt::InvalidEvent:
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
        connect(m_backend, SIGNAL(downloadMessage(int, const QString &)),
                m_downloadWidget, SLOT(updateDownloadMessage(int, const QString &)));
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
    initDownloadWidget();
    initCommitWidget();
    m_backend->commitChanges();
}

void UpdaterWindow::reload()
{
    m_backend->reloadCache();
    m_updaterWidget->refresh();
    m_statusWidget->updateStatus();
    setActionsEnabled(true);
    reloadActions();

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
    reloadActions();
}

void UpdaterWindow::reloadActions()
{
    QApt::PackageList changedList = m_backend->markedPackages();

    m_updateAction->setEnabled(true);
    m_applyAction->setEnabled(!changedList.isEmpty());

    m_undoAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_redoAction->setEnabled(!m_backend->isRedoStackEmpty());
    m_revertAction->setEnabled(!m_backend->isUndoStackEmpty());
}

void UpdaterWindow::setActionsEnabled(bool enabled)
{
    m_updateAction->setEnabled(enabled);
    m_applyAction->setEnabled(enabled);
    m_undoAction->setEnabled(enabled);
    m_redoAction->setEnabled(enabled);
    m_revertAction->setEnabled(enabled);
}

#include "UpdaterWindow.moc"
