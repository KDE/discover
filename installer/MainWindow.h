/***************************************************************************
 *   Copyright © 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

// KDE includes
#include <KService>

// Own includes
#include "../libmuon/MuonMainWindow.h"

class AbstractResourcesBackend;
class LaunchListModel;
class QAbstractItemView;
class QModelIndex;
class QSplitter;
class QStackedWidget;
class QStandardItem;
class QStandardItemModel;
class QTreeView;

class KMessageWidget;
class KService;
class KVBox;

class ApplicationBackend;
class ApplicationLauncher;
class ProgressView;
class ViewSwitcher;

class MainWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

private:
    AbstractResourcesBackend *m_appBackend;
    QSplitter *m_mainWidget;
    QStackedWidget *m_viewStack;
    QWidget *m_busyWidget;
    ViewSwitcher *m_viewSwitcher;
    QStandardItemModel *m_viewModel;
    QHash<QModelIndex, QWidget *> m_viewHash;
    KAction *m_loadSelectionsAction;
    KAction *m_saveSelectionsAction;
    KMessageWidget *m_launcherMessage;
    ApplicationLauncher *m_appLauncher;
    ProgressView *m_progressView;
    QStandardItem *m_progressItem;

    int m_transactionCount;
    LaunchListModel* m_launches;

private Q_SLOTS:
    void initGUI();
    void initObject();
    void loadSplitterSizes();
    void saveSplitterSizes();
    void clearViews();
    void populateViews();
    void changeView(const QModelIndex &index);
    void selectFirstRow(const QAbstractItemView *itemView);
    void runSourcesEditor();
    void sourcesEditorFinished();
    void showLauncherMessage();
    void launchSingleApp();
    void showAppLauncher();
    void onAppLauncherClosed();
    void clearMessageActions();
    void transactionAdded();
    void transactionRemoved();
    void addProgressItem();
    void removeProgressItem();
};

#endif
