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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class KAction;
class KDialog;
class KMessageWidget;

class ChangelogWidget;
class ProgressWidget;
class UpdaterSettingsDialog;
class UpdaterWidget;

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    ProgressWidget *m_progressWidget;
    UpdaterWidget *m_updaterWidget;
    ChangelogWidget *m_changelogWidget;
    UpdaterSettingsDialog *m_settingsDialog;
    KDialog *m_historyDialog;
    KMessageWidget *m_powerMessage;

    KAction *m_applyAction;
    KAction *m_createDownloadListAction;
    KAction *m_downloadListAction;
    KAction *m_loadArchivesAction;
    KAction *m_historyAction;

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
    void editSettings();
    void closeSettingsDialog();
    void showHistoryDialog();
    void closeHistoryDialog();
    void checkPlugState();
    void updatePlugState(bool plugged);
};

#endif // MAINWINDOW_H
