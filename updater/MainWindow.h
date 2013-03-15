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

class ResourcesUpdatesModel;
class KAction;
class KDialog;
class KMessageWidget;
class KProcess;
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
    ResourcesUpdatesModel* m_updater;

    ProgressWidget *m_progressWidget;
    UpdaterWidget *m_updaterWidget;
    UpdaterSettingsDialog *m_settingsDialog;
    KMessageWidget *m_powerMessage;
    KAction *m_applyAction;
    QMenu* m_moreMenu;

    virtual void setActionsEnabled(bool enabled = true);

private Q_SLOTS:
    void initGUI();
    void initBackend();
    void setupActions();
    void setupBackendsActions();
    void editSettings();
    void closeSettingsDialog();
    void checkPlugState();
    void updatePlugState(bool plugged);
    void progressingChanged();
};

#endif // MAINWINDOW_H
