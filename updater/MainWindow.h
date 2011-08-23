#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class KAction;

class UpdaterWidget;

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    UpdaterWidget *m_updaterWidget;

    KAction *m_applyAction;

private Q_SLOTS:
    void initGUI();
    void initObject();
    void setupActions();
    void reload();
    void setActionsEnabled(bool enabled = true);
};

#endif // MAINWINDOW_H
