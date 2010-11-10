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

class QModelIndex;
class QSplitter;
class QStackedWidget;
class QStandardItemModel;
class QTreeView;

class Application;
class ViewSwitcher;

namespace QApt
{
    class Backend;
}

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
   AppView = 1,
   /// An ApplicationView that has a Categorical homepage
   CatView = 2,
   /// A view for showing history
   HistoryView = 3
};

class ApplicationWindow : public MuonMainWindow
{
    Q_OBJECT
public:
    ApplicationWindow();
    virtual ~ApplicationWindow();

    int maxPopconScore() const;
    QList<Application *> applicationList() const;

private:
    QSplitter *m_mainWidget;
    QStackedWidget *m_viewStack;
    ViewSwitcher *m_viewSwitcher;
    QStandardItemModel *m_viewModel;
    QHash<QModelIndex, QWidget *> m_viewHash;

    int m_powerInhibitor;
    QList<Application *> m_appList;
    int m_maxPopconScore;

private Q_SLOTS:
    void initGUI();
    void reload();
    void populateAppList();
    void populateViews();
    void changeView(const QModelIndex &index);
};

#endif
