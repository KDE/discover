/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef RESOURCESPROXYMODEL_H
#define RESOURCESPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QStringList>

#include <Category/Category.h>

#include "libmuonprivate_export.h"
#include "AbstractResource.h"

namespace QApt {
    class Backend;
}

class Application;

class MUONPRIVATE_EXPORT ResourcesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel)
    Q_PROPERTY(bool shouldShowTechnical READ shouldShowTechnical WRITE setShouldShowTechnical)
    Q_PROPERTY(Category* filteredCategory READ filteredCategory WRITE setFiltersFromCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString originFilter READ originFilter WRITE setOriginFilter)
    Q_PROPERTY(bool isShowingTechnical READ shouldShowTechnical WRITE setShouldShowTechnical)
    Q_PROPERTY(bool isSortingByRelevancy READ sortingByRelevancy WRITE setSortByRelevancy)
    Q_PROPERTY(AbstractResource::State stateFilter READ stateFilter WRITE setStateFilter NOTIFY stateFilterChanged)
    Q_PROPERTY(QString mimeTypeFilter READ mimeTypeFilter WRITE setMimeTypeFilter)
    Q_PROPERTY(QString search READ lastSearch WRITE setSearch)
public:
    explicit ResourcesProxyModel(QObject *parent=0);

    void setSearch(const QString &text);
    QString lastSearch() const;
    void setOriginFilter(const QString &origin);
    QString originFilter() const;
    void setFiltersFromCategory(Category *category);
    void setShouldShowTechnical(bool show);
    bool shouldShowTechnical() const;
    void setSortByRelevancy(bool sort);
    bool sortingByRelevancy() const;
    bool isFilteringBySearch() const;
    void setStateFilter(AbstractResource::State s);
    void setFilterActive(bool filter);
    AbstractResource::State stateFilter() const;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    Category* filteredCategory() const;
    
    QString mimeTypeFilter() const;
    void setMimeTypeFilter(const QString& mime);

    virtual void setSourceModel(QAbstractItemModel *sourceModel);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private Q_SLOTS:
    void refreshSearch();

private:
    QString m_lastSearch;
    QList<AbstractResource*> m_searchResults;
    QList<QPair<FilterType, QString> > m_andFilters;
    QList<QPair<FilterType, QString> > m_orFilters;
    QList<QPair<FilterType, QString> > m_notFilters;
    QHash<int, QVariant> m_roleFilters;

    bool m_sortByRelevancy;
    bool m_filterBySearch;
    Category* m_filteredCategory;
    AbstractResource::State m_stateFilter;
    QString m_filteredMimeType;

Q_SIGNALS:
    void invalidated();
    void categoryChanged();
    void stateFilterChanged();
};

#endif
