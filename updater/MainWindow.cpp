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
#include <KToolBar>

// Own includes
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>
#include "ChangelogWidget.h"
#include "ProgressWidget.h"
#include "config/UpdaterSettingsDialog.h"
#include "UpdaterWidget.h"
#include "KActionMessageWidget.h"

MainWindow::MainWindow()
    : MuonMainWindow()
    , m_settingsDialog(nullptr)
{
    ResourcesModel *m = new ResourcesModel(this);
    m->registerBackendByName("muon-dummybackend");
    
    m_updater = new ResourcesUpdatesModel(this);
    connect(m_updater, SIGNAL(progressingChanged()), SLOT(progressingChanged()));
    connect(m_updater, SIGNAL(updatesFinnished()), SLOT(updatesFinished()));

    initGUI();
}

void MainWindow::initGUI()
{
    ResourcesModel* m = ResourcesModel::global();
    setWindowTitle(i18nc("@title:window", "Software Updates"));
    for(AbstractResourcesBackend* b : m->backends())
        b->integrateMainWindow(this);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    m_powerMessage = new KMessageWidget(mainWidget);
    m_powerMessage->setText(i18nc("@info Warning to plug in laptop before updating",
                                  "It is safer to plug in the power adapter before updating."));
    m_powerMessage->hide();
    m_powerMessage->setMessageType(KMessageWidget::Warning);
    checkPlugState();

    m_progressWidget = new ProgressWidget(mainWidget);
    m_progressWidget->setTransaction(m_updater);
    m_progressWidget->hide();

    m_updaterWidget = new UpdaterWidget(mainWidget);
    m_updaterWidget->setEnabled(false);

    m_changelogWidget = new ChangelogWidget(this);
    m_changelogWidget->hide();
    connect(m_updaterWidget, SIGNAL(selectedResourceChanged(AbstractResource*)),
            m_changelogWidget, SLOT(setResource(AbstractResource*)));

    mainLayout->addWidget(m_powerMessage);
//     mainLayout->addWidget(m_distUpgradeMessage);
    mainLayout->addWidget(m_progressWidget);
    mainLayout->addWidget(m_updaterWidget);
    mainLayout->addWidget(m_changelogWidget);

    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);
    setupActions();

//     connect(m_updater, SIGNAL(reloadStarted()), SLOT(startedReloading()));
    connect(m, SIGNAL(backendsChanged()), SLOT(finishedReloading()));
    connect(m, SIGNAL(allInitialized()), SLOT(initBackend()));
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Downloads and installs updates", "Install Updates"));
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    setActionsEnabled(false);

    setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));

    foreach (QAction* action, m_updater->messageActions()) {
        if (action->priority()==QAction::HighPriority) {
            KActionMessageWidget* w = new KActionMessageWidget(action, centralWidget());
            qobject_cast<QBoxLayout*>(centralWidget()->layout())->insertWidget(1, w);
        } else {
            toolBar("mainToolBar")->addAction(action);
        }
    }
}

void MainWindow::initBackend()
{
    m_updaterWidget->setBackend(m_updater);

    setActionsEnabled();
}

void MainWindow::progressingChanged()
{
    bool active = m_updater->isProgressing();
    QApplication::restoreOverrideCursor();
    m_progressWidget->setVisible(active);
    m_updaterWidget->setVisible(!active);
}

void MainWindow::updatesFinished()
{
    m_progressWidget->animatedHide();
    m_updaterWidget->setCurrentIndex(0);
    setActionsEnabled();
}

void MainWindow::startedReloading()
{
    setCanExit(false);
    setActionsEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->setResource(0);
}

void MainWindow::finishedReloading()
{
    QApplication::restoreOverrideCursor();
    checkPlugState();
    setActionsEnabled(true);
    setCanExit(true);
}

void MainWindow::setActionsEnabled(bool enabled)
{
    MuonMainWindow::setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    m_applyAction->setEnabled(m_updater->hasUpdates());
}

void MainWindow::startCommit()
{
    setActionsEnabled(false);
    m_updaterWidget->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_changelogWidget->animatedHide();

    m_updater->updateAll();
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
