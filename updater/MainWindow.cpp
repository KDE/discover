#include "MainWindow.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>

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

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    setActionsEnabled(false);

    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::reload()
{
    m_canExit = false;

    m_updaterWidget->reload();

    m_canExit = true;
}
