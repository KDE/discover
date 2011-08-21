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

    UpdaterWidget *updaterWidget = new UpdaterWidget(this);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            updaterWidget, SLOT(setBackend(QApt::Backend *)));

    setCentralWidget(updaterWidget);
}

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}
