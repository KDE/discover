/***************************************************************************
 *   Copyright © 2010-2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "AvailableView.h"

// Qt includes
#include <QtWidgets/QStackedWidget>

// KDE includes
#include <KCategorizedSortFilterProxyModel>
#include <KLocalizedString>

// Own includes
#include <Category/Category.h>
#include <resources/ResourcesModel.h>
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "CategoryView/CategoryViewWidget.h"
#include "ResourceDetailsView/ResourceDetailsView.h"

AvailableView::AvailableView(QWidget *parent)
        : AbstractViewContainer(parent)
{

    m_categoryViewWidget = new CategoryViewWidget(m_viewStack);

    m_categoryViewWidget->setDisplayedCategory(nullptr);
    m_breadcrumbWidget->setRootItem(m_categoryViewWidget->breadcrumbItem());

    m_viewStack->addWidget(m_categoryViewWidget);
    m_viewStack->setCurrentWidget(m_categoryViewWidget);

    ResourcesModel *resourcesModel = ResourcesModel::global();
    connect(resourcesModel, SIGNAL(searchInvalidated()),
            m_breadcrumbWidget, SLOT(startSearch()));
    connect(m_categoryViewWidget, SIGNAL(registerNewSubView(AbstractViewBase*)),
            this, SLOT(registerNewSubView(AbstractViewBase*)));
    connect(m_categoryViewWidget, SIGNAL(switchToSubView(AbstractViewBase*)),
            this, SLOT(switchToSubView(AbstractViewBase*)));
}

void AvailableView::setResource(AbstractResource *res)
{
    // Check to see if a view for this app already exists
    if (m_currentPair.second == res) {
        QMetaObject::invokeMethod(this, "switchToSubView", Q_ARG(AbstractViewBase*, m_currentPair.first));
        return;
    }

    // Create one if not
    m_detailsView = new ResourceDetailsView(this);
    m_detailsView->setResource(res);
    m_currentPair.first = m_detailsView;

    connect(m_detailsView, SIGNAL(destroyed(QObject*)),
            this, SLOT(onSubViewDestroyed()));

    // Tell our parent that we can exist, so that they can forward it
    QMetaObject::invokeMethod(this, "registerNewSubView", Q_ARG(AbstractViewBase*, m_detailsView));
}
