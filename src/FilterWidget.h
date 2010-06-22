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

#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

// Qt includes
#include <QtCore/QModelIndex>
#include <QtGui/QDockWidget>

class QDockWidget;
class QListView;
class QStandardItemModel;
class QToolBox;
class QTreeView;

class KLineEdit;

namespace QApt {
    class Backend;
}

class FilterWidget : public QDockWidget
{
    Q_OBJECT
public:
    FilterWidget(QWidget *parent, QApt::Backend *backend);
    ~FilterWidget();

private:
    QApt::Backend *m_backend;

    KLineEdit *m_searchEdit;
    QToolBox *m_filterBox;
    QTreeView *m_categoriesList;
    QListView *m_statusList;
    QListView *m_originList;

    QStandardItemModel *m_categoryModel;
    QStandardItemModel *m_statusModel;
    QStandardItemModel *m_originModel;

private Q_SLOTS:
    void populateCategories();
    void populateStatuses();
//     void populateOrigins();

    void categoryActivated(const QModelIndex &index);
    void statusActivated(const QModelIndex &index);

signals:
    void filterByGroup(const QString &groupText);
    void filterByStatus(const QString &statusText);
};

#endif
