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

#include "FilterWidget.h"

// Qt includes
#include <QStandardItemModel>
#include <QtWidgets/QLabel>
#include <QListView>
#include <QToolBox>

// KDE includes
#include <KLocalizedString>

// QApt includes
#include <QApt/Backend>

// Own includes
#include "ArchitectureFilter.h"
#include "CategoryFilter.h"
#include "OriginFilter.h"
#include "StatusFilter.h"

FilterWidget::FilterWidget(QWidget *parent)
    : QDockWidget(parent)
    , m_backend(0)
{
    setFeatures(QDockWidget::NoDockWidgetFeatures);
    setWindowTitle(i18nc("@title:window", "Filter:"));

    m_filterBox = new QToolBox(this);
    m_filterBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_categoriesList = new QListView(this);
    connect(m_categoriesList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(categoryActivated(QModelIndex)));
    m_listViews.append(m_categoriesList);
    m_filterBox->addItem(m_categoriesList, QIcon(), i18nc("@title:tab", "By Category"));

    m_statusList = new QListView(this);
    connect(m_statusList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(statusActivated(QModelIndex)));
    m_listViews.append(m_statusList);
    m_filterBox->addItem(m_statusList, QIcon(), i18nc("@title:tab", "By Status"));

    m_originList = new QListView(this);
    connect(m_originList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(originActivated(QModelIndex)));
    m_listViews.append(m_originList);
    m_filterBox->addItem(m_originList, QIcon(), i18nc("@title:tab", "By Origin"));

    m_archList = new QListView(this);
    connect(m_archList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(architectureActivated(QModelIndex)));
    m_listViews.append(m_archList);
    m_filterBox->addItem(m_archList, QIcon(), i18nc("@title:tab", "By Architecture"));

    for (QListView *view : m_listViews) {
        view->setAlternatingRowColors(true);
    }

    setWidget(m_filterBox);
    setEnabled(false);
}

FilterWidget::~FilterWidget()
{
}

void FilterWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;

    populateFilters();

    setEnabled(true);
}

void FilterWidget::reload()
{
    for (FilterModel *filterModel : m_filterModels) {
        filterModel->clear();
    }

    populateFilters();
}

void FilterWidget::populateFilters()
{
    // Create filter models
    CategoryFilter *categoryFilter = new CategoryFilter(this, m_backend);
    m_filterModels.append(categoryFilter);
    m_categoriesList->setModel(categoryFilter);

    StatusFilter *statusFilter = new StatusFilter(this);
    m_filterModels.append(statusFilter);
    m_statusList->setModel(statusFilter);

    OriginFilter *originFilter = new OriginFilter(this, m_backend);
    m_filterModels.append(originFilter);
    m_originList->setModel(originFilter);

    ArchitectureFilter *archFilter = new ArchitectureFilter(this, m_backend);
    m_filterModels.append(archFilter);
    m_archList->setModel(archFilter);

    // Populate filter lists
    for (FilterModel *filterModel : m_filterModels) {
        filterModel->populate();
    }

    // Set the selected item of each filter list to "All"
    for (QListView *view : m_listViews) {
        selectFirstRow(view);
    }
}

void FilterWidget::categoryActivated(const QModelIndex &index)
{
    QString groupName = index.data(Qt::DisplayRole).toString();
    emit filterByGroup(groupName);
}

void FilterWidget::statusActivated(const QModelIndex &index)
{
    QApt::Package::State state = (QApt::Package::State)index.data(Qt::UserRole+1).toInt();
    emit filterByStatus(state);
}

void FilterWidget::originActivated(const QModelIndex &index)
{
    QString originName = index.data(Qt::DisplayRole).toString();
    emit filterByOrigin(originName);
}

void FilterWidget::architectureActivated(const QModelIndex &index)
{
    QString arch = index.data(Qt::UserRole+1).toString();
    emit filterByArchitecture(arch);
}

void FilterWidget::selectFirstRow(const QAbstractItemView *itemView)
{
    QModelIndex firstRow = itemView->model()->index(0, 0);
    itemView->selectionModel()->select(firstRow, QItemSelectionModel::Select);
}

#include "FilterWidget.moc"
