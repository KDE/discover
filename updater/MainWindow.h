#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:

private Q_SLOTS:
    void initGUI();
    void setupActions();
};

#endif // MAINWINDOW_H
