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
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KMessageBox>
#include <KMessageWidget>
#include <KProcess>
#include <KProtocolManager>
#include <KStandardDirs>
#include <Solid/Device>
#include <Solid/AcAdapter>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/Transaction>

// Own includes
#include <MuonBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>
#include "../libmuonapt/HistoryView/HistoryView.h"
#include "../libmuonapt/MuonStrings.h"
#include "../libmuonapt/QAptActions.h"
#include "ChangelogWidget.h"
#include "ProgressWidget.h"
#include "config/UpdaterSettingsDialog.h"
#include "UpdaterWidget.h"

MainWindow::MainWindow()
    : MuonMainWindow()
    , m_settingsDialog(nullptr)
    , m_historyDialog(nullptr)
    , m_checkerProcess(nullptr)
{
    MuonBackendsFactory f;
    m_apps = f.backend("muon-appsbackend");
    connect(m_apps, SIGNAL(backendReady()), SLOT(initBackend()));

    initGUI();
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

    m_distUpgradeMessage = new KMessageWidget(mainWidget);
    m_distUpgradeMessage->hide();
    m_distUpgradeMessage->setMessageType(KMessageWidget::Information);
    m_distUpgradeMessage->setText(i18nc("Notification when a new version of Kubuntu is available",
                                        "A new version of Kubuntu is available."));

    m_progressWidget = new ProgressWidget(mainWidget);
    m_progressWidget->hide();

    m_updaterWidget = new UpdaterWidget(mainWidget);
    m_updaterWidget->setEnabled(false);

    m_changelogWidget = new ChangelogWidget(this);
    m_changelogWidget->hide();
    connect(m_updaterWidget, SIGNAL(selectedPackageChanged(QApt::Package*)),
            m_changelogWidget, SLOT(setPackage(QApt::Package*)));

    mainLayout->addWidget(m_powerMessage);
    mainLayout->addWidget(m_distUpgradeMessage);
    mainLayout->addWidget(m_progressWidget);
    mainLayout->addWidget(m_updaterWidget);
    mainLayout->addWidget(m_changelogWidget);

    m_apps->integrateMainWindow(this);
    setupActions();
    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);

    checkDistUpgrade();
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    connect(QAptActions::self(), SIGNAL(checkForUpdates()), this, SLOT(checkForUpdates()));

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

    KAction *distUpgradeAction = new KAction(KIcon("system-software-update"), i18nc("@action", "Upgrade"), this);
    connect(distUpgradeAction, SIGNAL(activated()), this, SLOT(launchDistUpgrade()));

    m_distUpgradeMessage->addAction(distUpgradeAction);

    setActionsEnabled(false);

    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::initBackend()
{
    m_updaterWidget->setBackend(m_apps);
    m_updaterWidget->setEnabled(true);

    setActionsEnabled();
}

void MainWindow::transactionStatusChanged(QApt::TransactionStatus status)
{
    // FIXME: better support for transactions that do/don't need reloads
    switch (status) {
    case QApt::RunningStatus:
    case QApt::WaitingStatus:
        QApplication::restoreOverrideCursor();
        m_progressWidget->show();
        break;
    case QApt::FinishedStatus:
        m_progressWidget->animatedHide();
        m_updaterWidget->setEnabled(true);
        m_updaterWidget->setCurrentIndex(0);
        reload();
        setActionsEnabled();

        m_trans->deleteLater();
        m_trans = nullptr;
        break;
    default:
        break;
    }
}

void MainWindow::errorOccurred(QApt::ErrorCode error)
{
    switch (error) {
    case QApt::LockError:
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
    setCanExit(false);
    m_changelogWidget->stopPendingJobs();

    disconnect(m_updaterWidget, SIGNAL(selectedPackageChanged(QApt::Package*)),
               m_changelogWidget, SLOT(setPackage(QApt::Package*)));

    m_updaterWidget->reload();

    connect(m_updaterWidget, SIGNAL(selectedPackageChanged(QApt::Package*)),
            m_changelogWidget, SLOT(setPackage(QApt::Package*)));

    m_changelogWidget->setPackage(0);
    QApplication::restoreOverrideCursor();

    checkPlugState();

    setCanExit(true);
}

void MainWindow::setActionsEnabled(bool enabled)
{
    QAptActions::self()->setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    m_applyAction->setEnabled(backend()->areChangesMarked());
    m_updaterWidget->setEnabled(true);
}

void MainWindow::checkForUpdates()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->animatedHide();
    m_changelogWidget->stopPendingJobs();

    m_trans = backend()->updateCache();
    setupTransaction(m_trans);
    m_trans->run();
}

void MainWindow::startCommit()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->animatedHide();
    m_changelogWidget->stopPendingJobs();

    m_trans = backend()->commitChanges();
    setupTransaction(m_trans);
    m_trans->run();
}

void MainWindow::setupTransaction(QApt::Transaction *trans)
{
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor("http"));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, 0)));

    m_progressWidget->setTransaction(m_trans);

    connect(m_trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(transactionStatusChanged(QApt::TransactionStatus)));
    connect(m_trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            this, SLOT(errorOccurred(QApt::ErrorCode)));
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

void MainWindow::checkPlugState()
{
    const QList<Solid::Device> acAdapters = Solid::Device::listFromType(Solid::DeviceInterface::AcAdapter);

    if (acAdapters.isEmpty()) {
        updatePlugState(true);
        return;
    }
    
    bool isPlugged = false;

    for(Solid::Device device_ac : acAdapters) {
        Solid::AcAdapter* acAdapter = device_ac.as<Solid::AcAdapter>();
        isPlugged |= acAdapter->isPlugged();
        connect(acAdapter, SIGNAL(plugStateChanged(bool,QString)),
                this, SLOT(updatePlugState(bool)), Qt::UniqueConnection);
    }

    updatePlugState(isPlugged);
}

void MainWindow::updatePlugState(bool plugged)
{
    m_powerMessage->setVisible(!plugged);
}

void MainWindow::checkDistUpgrade()
{
    QString checkerFile = KStandardDirs::locate("data", "muon-notifier/releasechecker");

    m_checkerProcess = new KProcess(this);
    m_checkerProcess->setProgram(QStringList() << "/usr/bin/python" << checkerFile);
    connect(m_checkerProcess, SIGNAL(finished(int)), this, SLOT(checkerFinished(int)));
    m_checkerProcess->start();
}

void MainWindow::checkerFinished(int res)
{
    if (res == 0) {
        m_distUpgradeMessage->show();
    }

    m_checkerProcess->deleteLater();
    m_checkerProcess = nullptr;
}

void MainWindow::launchDistUpgrade()
{
    KProcess::startDetached(QStringList() << "python"
                            << "/usr/share/pyshared/UpdateManager/DistUpgradeFetcherKDE.py");
}

QApt::Backend* MainWindow::backend() const
{
    return qobject_cast<QApt::Backend*>(m_apps->property("backend").value<QObject*>());
}
