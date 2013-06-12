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

#include "ResourceViewWidget.h"

// Qt includes
#include <QtCore/QStringBuilder>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KComboBox>
#include <KLocale>

// Libmuon includes
#include <Category/Category.h>
#include <Transaction/Transaction.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>

// Own includes
#include "ResourceDelegate.h"
#include "ResourceDetailsView/ResourceDetailsView.h"
#include "BreadcrumbWidget/BreadcrumbItem.h"

ResourceViewWidget::ResourceViewWidget(QWidget *parent)
        : AbstractViewBase(parent)
        , m_canShowTechnical(false)
        , m_detailsView(0)
{
    m_searchable = true;
    m_proxyModel = new ResourcesProxyModel(this);
    m_proxyModel->setSortRole(ResourcesModel::SortableRatingRole);
    m_proxyModel->setSourceModel(ResourcesModel::global());

    QWidget *header = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setMargin(0);
    header->setLayout(headerLayout);

    m_headerIcon = new QLabel(header);
    m_headerLabel = new QLabel(header);

    m_techCheckBox = new QCheckBox(header);
    m_techCheckBox->setText(i18n("Show technical items"));
    m_techCheckBox->hide();
    connect(m_techCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(techCheckChanged(int)));

    QLabel *sortLabel = new QLabel(header);
    sortLabel->setText(i18n("Sort:"));
    m_sortCombo = new KComboBox(header);
    m_sortCombo->addItem(i18nc("@item:inlistbox", "By Name"), ResourcesModel::NameRole);
    m_sortCombo->addItem(i18nc("@item:inlistbox", "By Top Rated"), ResourcesModel::SortableRatingRole);
    m_sortCombo->addItem(i18nc("@item:inlistbox", "By Most Buzz"), ResourcesModel::RatingPointsRole);
    m_sortCombo->setCurrentIndex(1); // Top Rated index
    connect(m_sortCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(sortComboChanged(int)));

    headerLayout->addWidget(m_headerIcon);
    headerLayout->addWidget(m_headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(sortLabel);
    headerLayout->addWidget(m_sortCombo);
    headerLayout->addWidget(m_techCheckBox);

    m_treeView = new QTreeView(this);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);

    m_treeView->setModel(m_proxyModel);
    m_delegate = new ResourceDelegate(m_treeView);
    m_treeView->setItemDelegate(m_delegate);

    connect(m_proxyModel, SIGNAL(invalidated()),
            m_delegate, SLOT(invalidate()));
    connect(m_proxyModel, SIGNAL(invalidated()),
            this, SLOT(updateSortCombo()));

    m_layout->addWidget(header);
    m_layout->addWidget(m_treeView);

    connect(m_delegate, SIGNAL(infoButtonClicked(AbstractResource*)),
            this, SLOT(infoButtonClicked(AbstractResource*)));

    m_treeView->setSortingEnabled(true);

    m_crumb->setAssociatedView(this);
}

void ResourceViewWidget::setTitle(const QString &title)
{
    m_crumb->setText(title);
    m_headerLabel->setText(QLatin1String("<h2>") % title % "</h2>");
}

void ResourceViewWidget::setIcon(const QIcon &icon)
{
    m_crumb->setIcon(icon);
    m_headerIcon->setPixmap(icon.pixmap(24,24));
}

void ResourceViewWidget::setStateFilter(AbstractResource::State state)
{
    m_proxyModel->setStateFilter(state);
}

void ResourceViewWidget::setOriginFilter(const QString &origin)
{
    m_proxyModel->setOriginFilter(origin);
}

void ResourceViewWidget::setFiltersFromCategory(Category *category)
{
    m_proxyModel->setFiltersFromCategory(category);
}

void ResourceViewWidget::setShouldShowTechnical(bool show)
{
    m_proxyModel->setShouldShowTechnical(show);
    m_techCheckBox->setChecked(show);
}

void ResourceViewWidget::setCanShowTechnical(bool canShow)
{
    m_canShowTechnical = canShow;

    if (canShow) {
        m_techCheckBox->show();
    }
}

void ResourceViewWidget::search(const QString &text)
{
    m_proxyModel->sort(m_proxyModel->sortColumn(), Qt::AscendingOrder);
    m_proxyModel->setSearch(text);
}

void ResourceViewWidget::infoButtonClicked(AbstractResource *resource)
{
    // Check to see if a view for this app already exists
    if (m_currentPair.second == resource) {
        emit switchToSubView(m_currentPair.first);
        return;
    }

    // Create one if not
    m_detailsView = new ResourceDetailsView(this);
    m_detailsView->setResource(resource);
    m_currentPair.first = m_detailsView;

    connect(m_detailsView, SIGNAL(destroyed(QObject*)),
            this, SLOT(onSubViewDestroyed()));

    // Tell our parent that we can exist, so that they can forward it
    emit registerNewSubView(m_detailsView);
}

void ResourceViewWidget::onSubViewDestroyed()
{
    m_currentPair.first = 0;
    m_currentPair.second = 0;
}

void ResourceViewWidget::sortComboChanged(int index)
{
    m_proxyModel->setSortRole(m_sortCombo->itemData(index).toInt());
    int sortRole = m_proxyModel->sortRole();

    if (m_proxyModel->isFilteringBySearch()) {
        bool sortByRelevancy = (sortRole == -1) ? true : false;
        m_proxyModel->setSortByRelevancy(sortByRelevancy);
    }

    switch (sortRole) {
    case ResourcesModel::SortableRatingRole:
    case ResourcesModel::RatingPointsRole:
        m_proxyModel->sort(m_proxyModel->sortColumn(), Qt::DescendingOrder);
        break;
    default:
        m_proxyModel->sort(m_proxyModel->sortColumn(), Qt::AscendingOrder);
    }
}

void ResourceViewWidget::updateSortCombo()
{
    bool searching = m_proxyModel->isFilteringBySearch();
    int searchItemIndex = m_sortCombo->findData(-1, Qt::UserRole);

    if (searching && searchItemIndex == -1) {
        // Add search combobox item for sort by search relevancy
        m_sortCombo->addItem(i18nc("@item:inlistbox", "By Relevancy"), -1);
        searchItemIndex = m_sortCombo->findData(-1, Qt::UserRole);
        m_sortCombo->setCurrentIndex(searchItemIndex);
    } else if (searchItemIndex != -1) {
        // Remove relevancy item if we aren't searching anymore
        m_sortCombo->removeItem(searchItemIndex);
    }
}

void ResourceViewWidget::techCheckChanged(int state)
{
    setShouldShowTechnical(state == Qt::Checked);
}
