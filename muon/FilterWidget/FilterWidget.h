/***************************************************************************
 *   Copyright © 2010, 2011 Jonathan Thomas <echidnaman@kubuntu.org>       *
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
#include <QDockWidget>

#include <QApt/Package>

class QAbstractItemView;
class QDockWidget;
class QListView;
class QStandardItemModel;
class QToolBox;
class QTreeView;

class FilterModel;

namespace QApt
{
    class Backend;
}

class FilterWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit FilterWidget(QWidget *parent);
    ~FilterWidget();

private:
    QApt::Backend *m_backend;

    QToolBox *m_filterBox;
    QListView *m_categoriesList;
    QListView *m_statusList;
    QListView *m_originList;
    QListView *m_archList;

    QVector<QListView *> m_listViews;
    QVector<FilterModel *> m_filterModels;

    void selectFirstRow(const QAbstractItemView *itemView);

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void reload();

private Q_SLOTS:
    void populateFilters();

    void categoryActivated(const QModelIndex &index);
    void statusActivated(const QModelIndex &index);
    void originActivated(const QModelIndex &index);
    void architectureActivated(const QModelIndex &index);

signals:
    void filterByGroup(const QString &groupName);
    void filterByStatus(const QApt::Package::State state);
    void filterByOrigin(const QString &originName);
    void filterByArchitecture(const QString &architecture);
};

#endif
