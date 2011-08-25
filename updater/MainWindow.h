#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class KAction;

class ProgressWidget;
class UpdaterWidget;

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    ProgressWidget *m_progressWidget;
    UpdaterWidget *m_updaterWidget;

    KAction *m_applyAction;
    KAction *m_createDownloadListAction;
    KAction *m_downloadListAction;
    KAction *m_loadArchivesAction;

private Q_SLOTS:
    void initGUI();
    void initObject();
    void setupActions();
    void workerEvent(QApt::WorkerEvent event);
    void errorOccurred(QApt::ErrorCode error, const QVariantMap &args);
    void reload();
    void setActionsEnabled(bool enabled = true);
    void checkForUpdates();
    void startCommit();
};

#endif // MAINWINDOW_H
