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

#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QStringList>
#include <QQmlParserStatus>

#include <Category/Category.h>

#include "discovercommon_export.h"
#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"

class Transaction;
class AggregatedResultsStream;

class DISCOVERCOMMON_EXPORT ResourcesProxyModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Roles sortRole READ sortRole WRITE setSortRole NOTIFY sortRoleChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(Category* filteredCategory READ filteredCategory WRITE setFiltersFromCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString originFilter READ originFilter WRITE setOriginFilter)
    Q_PROPERTY(AbstractResource::State stateFilter READ stateFilter WRITE setStateFilter NOTIFY stateFilterChanged)
    Q_PROPERTY(QString mimeTypeFilter READ mimeTypeFilter WRITE setMimeTypeFilter)
    Q_PROPERTY(QString search READ lastSearch WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(QUrl resourcesUrl READ resourcesUrl WRITE setResourcesUrl NOTIFY resourcesUrlChanged)
    Q_PROPERTY(QString extending READ extends WRITE setExtends)
    Q_PROPERTY(bool allBackends READ allBackends WRITE setAllBackends)
    Q_PROPERTY(QVariantList subcategories READ subcategories NOTIFY subcategoriesChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
public:
    explicit ResourcesProxyModel(QObject* parent = nullptr);
    enum Roles {
        NameRole = Qt::UserRole,
        IconRole,
        CommentRole,
        StateRole,
        RatingRole,
        RatingPointsRole,
        RatingCountRole,
        SortableRatingRole,
        InstalledRole,
        ApplicationRole,
        OriginRole,
        DisplayOriginRole,
        CanUpgrade,
        PackageNameRole,
        IsTechnicalRole,
        CategoryRole,
        CategoryDisplayRole,
        SectionRole,
        MimeTypes,
        SizeRole,
        LongDescriptionRole
    };
    Q_ENUM(Roles)

    QHash<int, QByteArray> roleNames() const override;

    void setSearch(const QString &text);
    QString lastSearch() const;
    void setOriginFilter(const QString &origin);
    QString originFilter() const;
    void setFiltersFromCategory(Category *category);
    void setStateFilter(AbstractResource::State s);
    AbstractResource::State stateFilter() const;
    void setSortRole(Roles sortRole);
    Roles sortRole() const { return m_sortRole; }
    void setSortOrder(Qt::SortOrder sortOrder);
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    Category* filteredCategory() const;
    
    QString mimeTypeFilter() const;
    void setMimeTypeFilter(const QString& mime);

    QString extends() const;
    void setExtends(const QString &extends);

    QUrl resourcesUrl() const;
    void setResourcesUrl(const QUrl& resourcesUrl);

    bool allBackends() const;
    void setAllBackends(bool allBackends);

    QVariantList subcategories() const;

    QVariant data(const QModelIndex & index, int role) const override;
    int rowCount(const QModelIndex & parent = {}) const override;

    Q_SCRIPTABLE int indexOf(AbstractResource* res);
    Q_SCRIPTABLE AbstractResource* resourceAt(int row) const;

    bool isBusy() const { return m_currentStream != nullptr; }

    bool lessThan(AbstractResource* rl, AbstractResource* rr) const;
    void invalidateFilter();
    void invalidateSorting();

    void classBegin() override {}
    void componentComplete() override;

private Q_SLOTS:
    void refreshBackend(AbstractResourcesBackend* backend, const QVector<QByteArray>& properties);
    void refreshResource(AbstractResource* resource, const QVector<QByteArray>& properties);
    void removeResource(AbstractResource* resource);
private:
    void sortedInsertion(const QVector<AbstractResource*> &res);
    QVariant roleToValue(AbstractResource* res, int role) const;

    QVector<int> propertiesToRoles(const QVector<QByteArray>& properties) const;
    void addResources(const QVector<AbstractResource*> &res);
    void fetchSubcategories();
    void removeDuplicates(QVector<AbstractResource *>& newResources);

    Roles m_sortRole;
    Qt::SortOrder m_sortOrder;

    bool m_sortByRelevancy;
    bool m_setup = false;

    AbstractResourcesBackend::Filters m_filters;
    QVariantList m_subcategories;

    QVector<AbstractResource*> m_displayedResources;
    const QHash<int, QByteArray> m_roles;
    AggregatedResultsStream* m_currentStream;

Q_SIGNALS:
    void busyChanged(bool isBusy);
    void sortRoleChanged(int sortRole);
    void sortOrderChanged(Qt::SortOrder order);
    void categoryChanged();
    void stateFilterChanged();
    void searchChanged(const QString &search);
    void subcategoriesChanged(const QVariantList &subcategories);
    void resourcesUrlChanged(const QUrl &url);
};

#endif
