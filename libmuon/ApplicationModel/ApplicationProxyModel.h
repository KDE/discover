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

#ifndef APPLICATIONPROXYMODEL_H
#define APPLICATIONPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QString>

#include <LibQApt/Package>

#include <Category/Category.h>

#include "libmuonprivate_export.h"

namespace QApt {
    class Backend;
}

class Application;

class MUONPRIVATE_EXPORT ApplicationProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel)
    Q_PROPERTY(bool shouldShowTechnical READ shouldShowTechnical WRITE setShouldShowTechnical)
    Q_PROPERTY(Category* filteredCategory READ filteredCategory WRITE setFiltersFromCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString originFilter READ originFilter WRITE setOriginFilter)
public:
    explicit ApplicationProxyModel(QObject *parent=0);
    ~ApplicationProxyModel();

    void setBackend(QApt::Backend *backend);
    Q_SCRIPTABLE void search(const QString &text);
    void setStateFilter(QApt::Package::State state);
    QApt::Package::State stateFilter() const;
    void setOriginFilter(const QString &origin);
    QString originFilter() const;
    void setFiltersFromCategory(Category *category);
    void setShouldShowTechnical(bool show);
    bool shouldShowTechnical() const;
    void setSortByRelevancy(bool sort);
    bool sortingByRelevancy() const;
    bool isFilteringBySearch() const;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    Application *applicationAt(const QModelIndex &index) const;
    Category* filteredCategory() const;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QApt::Backend *m_backend;

    QString m_lastSearch;
    QApt::Package::State m_stateFilter;
    QString m_originFilter;
    QApt::PackageList m_packages;
    QList<QPair<FilterType, QString> > m_andFilters;
    QList<QPair<FilterType, QString> > m_orFilters;
    QList<QPair<FilterType, QString> > m_notFilters;

    bool m_sortByRelevancy;
    bool m_filterBySearch;
    bool m_showTechnical;
    Category* m_filteredCategory;

public Q_SLOTS:
    void refreshSearch();

Q_SIGNALS:
    void invalidated();
    void categoryChanged();
};

#endif
