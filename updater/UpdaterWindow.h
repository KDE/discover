/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef UPDATERWINDOW_H
#define UPDATERWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"

class QStackedWidget;

class KAction;

class DownloadWidget;
class CommitWidget;
class StatusWidget;
class UpdaterSettingsDialog;
class UpdaterWidget;

namespace QApt
{
    class Backend;
}

class UpdaterWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    UpdaterWindow();
    virtual ~UpdaterWindow();

private:
    QStackedWidget *m_stack;
    UpdaterWidget *m_updaterWidget;

    KAction *m_applyAction;
    KAction *m_createDownloadListAction;
    KAction *m_downloadListAction;
    KAction *m_loadArchivesAction;

    UpdaterSettingsDialog *m_settingsDialog;
    DownloadWidget *m_downloadWidget;
    CommitWidget *m_commitWidget;
    StatusWidget *m_statusWidget;


private Q_SLOTS:
    void initGUI();
    void initObject();
    void setupActions();
    void checkForUpdates();
    void downloadPackagesFromList();
    void workerEvent(QApt::WorkerEvent event);
    void errorOccurred(QApt::ErrorCode code, const QVariantMap &args);
    void initDownloadWidget();
    void initCommitWidget();
    void startCommit();
    void returnFromPreview();
    void reload();
    void setActionsEnabled(bool enabled = true);
    void editSettings();
    void closeSettingsDialog();
};

#endif
