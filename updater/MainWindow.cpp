/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QMenu>
#include <QMenuBar>
#include <QLayoutItem>

// KDE includes
#include <KActionCollection>
#include <KMessageBox>
#include <KMessageWidget>
#include <KProcess>
#include <KProtocolManager>
#include <KToolBar>
#include <KLocalizedString>
#warning TODO, waiting for this new API to finally be accepted
// #include <Solid/Power>
// #include <Solid/AcPluggedJob>

// Own includes
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/UIHelper.h>
#include "ProgressWidget.h"
#include "UpdaterWidget.h"
#include "KActionMessageWidget.h"
#include "ui_UpdaterCentralWidget.h"

MainWindow::MainWindow()
    : KXmlGuiWindow()
 {
    m_updater = new ResourcesUpdatesModel(this);
    connect(m_updater, SIGNAL(progressingChanged()), SLOT(progressingChanged()));

    setupActions();
    initGUI();
}

void MainWindow::initGUI()
{
    setWindowTitle(i18nc("@title:window", "Software Updates"));
    ResourcesModel* m = ResourcesModel::global();
    m->integrateMainWindow(this);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    m_powerMessage = new KMessageWidget(mainWidget);
    m_powerMessage->setText(i18nc("@info Warning to plug in laptop before updating",
                                  "It is safer to plug in the power adapter before updating."));
    m_powerMessage->setMessageType(KMessageWidget::Warning);
    checkPlugState();

    m_progressWidget = new ProgressWidget(m_updater, mainWidget);
    m_updaterWidget = new UpdaterWidget(m_updater, mainWidget);
    m_updaterWidget->setEnabled(false);
    connect(m_updaterWidget, SIGNAL(modelPopulated()),
            this, SLOT(setActionsEnabled()));

    Ui::UpdaterButtons buttonsUi;
    QWidget* buttons = new QWidget(this);
    buttonsUi.setupUi(buttons);
    buttonsUi.more->setMenu(m_moreMenu);
    buttonsUi.apply->setDefaultAction(m_applyAction);
    buttonsUi.quit->setDefaultAction(action("file_quit"));

    mainLayout->addWidget(m_powerMessage);
    mainLayout->addWidget(m_updaterWidget);
    mainLayout->addWidget(m_progressWidget);
    mainLayout->addWidget(buttons);

    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);
    progressingChanged();

    connect(m, SIGNAL(allInitialized()), SLOT(initBackend()));
    menuBar()->setVisible(false);
    toolBar()->setVisible(false);

#warning TODO, waiting for this new API to finally be accepted
//     connect(Solid::Power::self(), SIGNAL(acPluggedChanged(bool)), SLOT(updatePlugState(bool)));
    updatePlugState(true);
}

void MainWindow::setupActions()
{
    QAction *quitAction = KStandardAction::quit(QCoreApplication::instance(), SLOT(quit()), actionCollection());
    actionCollection()->addAction("file_quit", quitAction);

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Downloads and installs updates", "Install Updates"));
    connect(m_applyAction, SIGNAL(triggered()), m_updater, SLOT(updateAll()));
    m_applyAction->setEnabled(false);

    setActionsEnabled(false);

//     setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
    setupGUI(StandardWindowOption((KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar) & ~KXmlGuiWindow::ToolBar));

    m_moreMenu = new QMenu(this);
    m_advancedMenu = new QMenu(i18n("Advanced..."), m_moreMenu);
    setupBackendsActions();
}

void MainWindow::initBackend()
{
    setupBackendsActions();

    setActionsEnabled();
}

static bool containsAction(QAction* action, QBoxLayout* l)
{
    for(int i=0, count=l->count(); i<count; ++i) {
        KActionMessageWidget* w = qobject_cast<KActionMessageWidget*>(l->itemAt(i)->widget());
        if(w && w->action() == action) {
            return true;
        }
    }
    return false;
}

void MainWindow::setupBackendsActions()
{
    m_advancedMenu->clear();
    m_moreMenu->clear();

    QList<QAction*> highActions = UIHelper::setupMessageActions(m_moreMenu, m_advancedMenu, ResourcesModel::global()->messageActions());
    for (QAction* action: highActions) {
        if(!containsAction(action, qobject_cast<QBoxLayout*>(centralWidget()->layout()))) {
            KActionMessageWidget* w = new KActionMessageWidget(action, centralWidget());
            qobject_cast<QBoxLayout*>(centralWidget()->layout())->insertWidget(1, w);
        }
    }

    if (!m_moreMenu->isEmpty())
        m_moreMenu->addSeparator();
    m_moreMenu->addAction(actionCollection()->action("options_configure"));
    m_moreMenu->addAction(actionCollection()->action("options_configure_keybinding"));
    m_moreMenu->addSeparator();
    m_moreMenu->addMenu(m_advancedMenu);
    m_moreMenu->addSeparator();
    m_moreMenu->addAction(actionCollection()->action("help_about_app"));
    m_moreMenu->addAction(actionCollection()->action("help_about_kde"));
    m_moreMenu->addAction(actionCollection()->action("help_report_bug"));
}
QSize MainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(900, 500));
}

void MainWindow::progressingChanged()
{
    QApplication::restoreOverrideCursor();
    m_updaterWidget->setCurrentIndex(0);

    bool active = m_updater->isProgressing();
    m_progressWidget->setVisible(active);
//     m_updaterWidget->setVisible(!active);
    setActionsEnabled(!active);
}

void MainWindow::setActionsEnabled(bool enabled)
{
    foreach (QAction* a, actionCollection()->actions()) {
        a->setEnabled(enabled);
    }
    m_applyAction->setEnabled(enabled && m_updater->hasUpdates());
}

void MainWindow::checkPlugState()
{
#warning TODO, waiting for this new API to finally be accepted
//     Solid::AcPluggedJob* job = Solid::Power::isAcPlugged();
//     connect(job, &Solid::AcPluggedJob::result, this, [=]() { updatePlugState(job->isPlugged()); });
}

void MainWindow::updatePlugState(bool plugged)
{
    m_powerMessage->setVisible(!plugged);
}

bool MainWindow::queryClose()
{
    return !m_updater->isProgressing() && !ResourcesModel::global()->isBusy();
}
