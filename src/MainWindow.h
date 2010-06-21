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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <kxmlguiwindow.h>

#include <libqapt/globals.h>

class QSplitter;
class QStackedWidget;
class QToolBox;
class KAction;

class FilterWidget;
class ManagerWidget;
class ReviewWidget;
class DownloadWidget;
class CommitWidget;

namespace QApt {
    class Backend;
}

/**
 * This class serves as the main window for Muon.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Jonathan Thomas <echidnaman@kubuntu.org>
 * @version 0.1
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow();

    virtual ~MainWindow();

private:
    QApt::Backend *m_backend;

    QStackedWidget *m_stack;
    QSplitter *m_mainWidget;
    KAction *m_updateAction;
    KAction *m_upgradeAction;
    KAction *m_previewAction;
    KAction *m_applyAction;

    FilterWidget *m_filterBox;
    ManagerWidget *m_managerWidget;
    ReviewWidget *m_reviewWidget;
    DownloadWidget *m_downloadWidget;
    CommitWidget *m_commitWidget;

public Q_SLOTS:

private Q_SLOTS:
    void setupActions();
    void slotQuit();
    void slotUpgrade();
    void slotUpdate();
    void workerEvent(QApt::WorkerEvent event);
    void previewChanges();
    void startCommit();
    void initDownloadWidget();
    void initCommitWidget();
    void reload();
    void reloadActions();
};

#endif // _MUON_H_
