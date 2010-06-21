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
    : KXmlGuiWindow()
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

//     QLabel* packageCountLabel = new QLabel(this);
//     packageCountLabel->setText(i18np("%1 item available", "%1 items available", m_backend->packageCount()));
//     statusBar()->addWidget(packageCountLabel);
    statusBar()->hide();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupActions()
{
    // local - Destroys all sub-windows and exits
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);

    KAction *upgradeAction = actionCollection()->addAction("upgrade");
    upgradeAction->setIcon(KIcon("system-software-update"));
    upgradeAction->setText("Upgrade");
    connect(upgradeAction, SIGNAL(triggered()), this, SLOT(slotUpgrade()));
    if (m_backend->upgradeablePackages().isEmpty()) {
        upgradeAction->setEnabled(false);
    }

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
    reviewChanges();
}
void MainWindow::workerEvent(QApt::WorkerEvent event)
{
    switch (event) {
        case QApt::CacheUpdateStarted:
            m_downloadWidget->clear();
            m_downloadWidget->setHeaderText(i18n("<b>Updating software sources</b>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::CacheUpdateFinished:
            m_stack->setCurrentWidget(m_mainWidget);
            break;
        case QApt::PackageDownloadStarted:
            m_downloadWidget->clear();
            m_downloadWidget->setHeaderText(i18n("<b>Downloading Packages</b>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            kDebug() << "set current widget to downloadwidget";
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::PackageDownloadFinished:
            m_stack->setCurrentWidget(m_mainWidget);
            break;
        case QApt::CommitChangesStarted:
            m_commitWidget->clear();
            m_stack->setCurrentWidget(m_commitWidget);
            break;
        case QApt::CommitChangesFinished:
            m_managerWidget->reload();
            m_stack->setCurrentWidget(m_mainWidget);
            break;
    }
}

void MainWindow::reviewChanges()
{
    if (!m_reviewWidget) {
        m_reviewWidget = new ReviewWidget(m_stack, m_backend);
        connect(m_reviewWidget, SIGNAL(startCommit()), this, SLOT(startCommit()));
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

#include "MainWindow.moc"
