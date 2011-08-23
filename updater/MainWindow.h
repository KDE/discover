#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class UpdaterWidget;

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    UpdaterWidget *m_updaterWidget;

private Q_SLOTS:
    void initGUI();
    void setupActions();
    void reload();
};

#endif // MAINWINDOW_H
