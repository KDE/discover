#include "MainWindow.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KAction>
#include <KActionCollection>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "UpdaterWidget.h"

MainWindow::MainWindow()
    : MuonMainWindow()
{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

void MainWindow::initGUI()
{
    setWindowTitle(i18nc("@title:window", "Software Updates"));

    m_updaterWidget = new UpdaterWidget(this);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_updaterWidget, SLOT(setBackend(QApt::Backend *)));

    setupActions();

    setCentralWidget(m_updaterWidget);
}

void MainWindow::initObject()
{
    MuonMainWindow::initObject();
    setActionsEnabled(); //Get initial enabled/disabled state
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Downloads and installs updates", "Install Updates"));
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    setActionsEnabled(false);

    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::reload()
{
    m_canExit = false;

    m_updaterWidget->reload();

    m_canExit = true;
}

void MainWindow::setActionsEnabled(bool enabled)
{
    MuonMainWindow::setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    //m_downloadListAction->setEnabled(isConnected());

    m_applyAction->setEnabled(m_backend->areChangesMarked());
    m_undoAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_redoAction->setEnabled(!m_backend->isRedoStackEmpty());
    m_revertAction->setEnabled(!m_backend->isUndoStackEmpty());
}
