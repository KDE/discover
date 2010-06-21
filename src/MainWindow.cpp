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
#include <QtGui/QLabel>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBox>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KConfigDialog>
#include <KLocale>
#include <KStandardAction>
#include <KStatusBar>
#include <KToolBar>
#include <KDebug>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "CommitWidget.h"
#include "DownloadWidget.h"
#include "FilterWidget.h"
#include "ManagerWidget.h"
#include "ReviewWidget.h"

MainWindow::MainWindow()
    : KXmlGuiWindow(0)
    , m_backend(0)
    , m_stack(0)
    , m_reviewWidget(0)
    , m_downloadWidget(0)
    , m_commitWidget(0)

{
    m_backend = new QApt::Backend;
    m_backend->init();
    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode, const QVariantMap&)),
            this, SLOT(errorOccurred(QApt::ErrorCode, const QVariantMap&)));
    connect(m_backend, SIGNAL(questionOccurred(QApt::WorkerQuestion, const QVariantMap&)),
            this, SLOT(questionOccurred(QApt::WorkerQuestion, const QVariantMap&)));
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(reloadActions()));

    m_stack = new QStackedWidget;
    setCentralWidget(m_stack);

    m_filterBox = new FilterWidget(m_stack, m_backend);
    m_managerWidget = new ManagerWidget(m_stack, m_backend);
    connect (m_filterBox, SIGNAL(filterByGroup(const QString&)),
             m_managerWidget, SLOT(filterByGroup(const QString&)));
    connect (m_filterBox, SIGNAL(filterByStatus(const QString&)),
             m_managerWidget, SLOT(filterByStatus(const QString&)));

    m_mainWidget = new QSplitter(this);
    m_mainWidget->setOrientation(Qt::Horizontal);
    m_mainWidget->addWidget(m_filterBox);
    m_mainWidget->addWidget(m_managerWidget);
    // TODO: Store/restore on app exit/restore
    QList<int> sizes;
    sizes << 115 << (this->width() - 115);
    m_mainWidget->setSizes(sizes);

    m_stack->addWidget(m_mainWidget);
    m_stack->setCurrentWidget(m_mainWidget);

    setupActions();

    QLabel* packageCountLabel = new QLabel(this);
    packageCountLabel->setText(i18np("%1 package available", "%1 packages available", m_backend->packageCount()));
    statusBar()->addWidget(packageCountLabel);
    statusBar()->show();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupActions()
{
    // local - Destroys all sub-windows and exits
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);

    KAction *updateAction = actionCollection()->addAction("update");
    updateAction->setIcon(KIcon("download"));
    updateAction->setText("Check for Updates");
    connect(updateAction, SIGNAL(triggered()), this, SLOT(slotUpdate()));

    m_upgradeAction = actionCollection()->addAction("upgrade");
    m_upgradeAction->setIcon(KIcon("system-software-update"));
    m_upgradeAction->setText("Upgrade");
    connect(m_upgradeAction, SIGNAL(triggered()), this, SLOT(slotUpgrade()));

    m_previewAction = actionCollection()->addAction("preview");
    m_previewAction->setIcon(KIcon("document-preview-archive"));
    m_previewAction->setText("Preview Changes");
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText("Apply Changes");
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    reloadActions(); //Get initial enabled/disabled state
    setupGUI();
}

void MainWindow::slotQuit()
{
    //Settings::self()->writeConfig();
    KApplication::instance()->quit();
}

void MainWindow::slotUpgrade()
{
    m_backend->markPackagesForDistUpgrade();
    previewChanges();
}

void MainWindow::slotUpdate()
{
    initDownloadWidget();
    m_backend->updateCache();
}

void MainWindow::workerEvent(QApt::WorkerEvent event)
{
    switch (event) {
        case QApt::CacheUpdateStarted:
            this->toolBar("mainToolBar")->setEnabled(false);
            m_downloadWidget->clear();
            m_downloadWidget->setHeaderText(i18n("<b>Updating software sources</b>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::CacheUpdateFinished:
        case QApt::CommitChangesFinished:
            reload();
            break;
        case QApt::PackageDownloadStarted:
            this->toolBar("mainToolBar")->setEnabled(false);
            m_downloadWidget->clear();
            m_downloadWidget->setHeaderText(i18n("<b>Downloading Packages</b>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::CommitChangesStarted:
            m_commitWidget->clear();
            m_stack->setCurrentWidget(m_commitWidget);
            break;
        case QApt::PackageDownloadFinished:
        case QApt::InvalidEvent:
        default:
            break;
    }
}

void MainWindow::previewChanges()
{
    if (!m_reviewWidget) {
        m_reviewWidget = new ReviewWidget(m_stack, m_backend);
        m_stack->addWidget(m_reviewWidget);
    }

    m_stack->setCurrentWidget(m_reviewWidget);
}

void MainWindow::startCommit()
{
    initDownloadWidget();
    initCommitWidget();
    m_backend->commitChanges();
}

void MainWindow::initDownloadWidget()
{
    if (!m_downloadWidget) {
        m_downloadWidget = new DownloadWidget(this);
        m_stack->addWidget(m_downloadWidget);
        connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                m_downloadWidget, SLOT(updateDownloadProgress(int, int, int)));
        connect(m_backend, SIGNAL(downloadMessage(int, const QString&)),
                m_downloadWidget, SLOT(updateDownloadMessage(int, const QString&)));
    }
}

void MainWindow::initCommitWidget()
{
    if (!m_commitWidget) {
        m_commitWidget = new CommitWidget(this);
        m_stack->addWidget(m_commitWidget);
        connect(m_backend, SIGNAL(commitProgress(const QString&, int)),
                m_commitWidget, SLOT(updateCommitProgress(const QString&, int)));
    }
}

void MainWindow::reload()
{
    m_managerWidget->reload();
    toolBar("mainToolBar")->setEnabled(true);
    m_stack->setCurrentWidget(m_mainWidget);
    reloadActions();

    // No need to keep these around in memory.
    delete m_downloadWidget;
    delete m_commitWidget;
    m_downloadWidget = 0;
    m_commitWidget = 0;
}

void MainWindow::reloadActions()
{
    QApt::PackageList upgradeableList = m_backend->upgradeablePackages();
    QApt::PackageList changedList = m_backend->markedPackages();

    m_upgradeAction->setEnabled(!upgradeableList.isEmpty());
    m_previewAction->setEnabled(!changedList.isEmpty());
    m_applyAction->setEnabled(!changedList.isEmpty());
}

#include "MainWindow.moc"
