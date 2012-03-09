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

#ifndef APPLICATIONWINDOW_H
#define APPLICATIONWINDOW_H

// Own includes
#include "../libmuon/MuonMainWindow.h"
#include <KService>

class QAbstractItemView;
class QModelIndex;
class QSplitter;
class QStackedWidget;
class QStandardItem;
class QStandardItemModel;
class QTreeView;

class KMessageWidget;
class KService;

class Application;
class ApplicationBackend;
class ApplicationLauncher;
class ProgressView;
class ViewSwitcher;

enum ViewModelRole {
    /// A role for storing ViewType
    ViewTypeRole = Qt::UserRole + 1,
    /// A role for storing origin filter data
    OriginFilterRole = Qt::UserRole + 2,
    /// A role for storing state filter data
    StateFilterRole = Qt::UserRole + 3
};

enum ViewType {
    /// An invalid value
    InvalidView = 0,
    /// A simple ApplicationView that is filterable by status or origin
    AppView,
    /// An ApplicationView that has a Categorical homepage
    CatView,
    /// A CategoryView showing subcategories
    SubCatView,
    /// A view for showing history
    History,
    /// A view for showing in-progress transactions
    Progress
};

class ApplicationWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    ApplicationWindow();
    ~ApplicationWindow();

    ApplicationBackend *appBackend() const;

private:
    ApplicationBackend *m_appBackend;
    QSplitter *m_mainWidget;
    QStackedWidget *m_viewStack;
    ViewSwitcher *m_viewSwitcher;
    QStandardItemModel *m_viewModel;
    QHash<QModelIndex, QWidget *> m_viewHash;
    KAction *m_loadSelectionsAction;
    KAction *m_saveSelectionsAction;
    KMessageWidget *m_launcherMessage;
    ApplicationLauncher *m_appLauncher;
    ProgressView *m_progressView;
    QStandardItem *m_progressItem;

    QVector<KService::Ptr> m_launchableApps;

private Q_SLOTS:
    void initGUI();
    void initObject();
    void loadSplitterSizes();
    void saveSplitterSizes();
    void setupActions();
    void clearViews();
    void checkForUpdates();
    void setActionsEnabled(bool enabled = true);
    void workerEvent(QApt::WorkerEvent event);
    void populateViews();
    void changeView(const QModelIndex &index);
    void selectFirstRow(const QAbstractItemView *itemView);
    void runSourcesEditor();
    void sourcesEditorFinished(int reload);
    void showLauncherMessage();
    void launchSingleApp();
    void showAppLauncher();
    void onAppLauncherClosed();
    void clearMessageActions();
    void addProgressItem();
    void removeProgressItem();
};

#endif
