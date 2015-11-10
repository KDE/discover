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

#ifndef HISTORYVIEW_H
#define HISTORYVIEW_H

#include <QtCore/QHash>

#include <QWidget>

class QStandardItem;
class QStandardItemModel;
class QTimer;
class QTreeView;
class QLineEdit;
class QComboBox;

namespace QApt {
    class History;
}

class HistoryProxyModel;

class HistoryView : public QWidget
{
    Q_OBJECT
public:
    enum ComboItems {
        AllChangesItem = 0,
        InstallationsItem = 1,
        UpdatesItem = 2,
        RemovalsItem = 3
    };
    enum PastActions {
        InvalidAction = 0,
        InstalledAction = 1,
        UpgradedAction = 2,
        DowngradedAction = 3,
        RemovedAction = 4,
        PurgedAction = 5
    };
    HistoryView(QWidget *parent);

    QSize sizeHint() const;

private:
    QApt::History *m_history;
    QStandardItemModel *m_historyModel;
    HistoryProxyModel *m_proxyModel;
    QHash<QString, QStandardItem *> m_categoryHash;

    QLineEdit *m_searchEdit;
    QTimer *m_searchTimer;
    QComboBox *m_filterBox;
    QTreeView *m_historyView;

private Q_SLOTS:
    void setStateFilter(int index);
    void startSearch();
};

#endif
