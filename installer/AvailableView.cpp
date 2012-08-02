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
#include <QtGui/QStackedWidget>

// KDE includes
#include <KCategorizedSortFilterProxyModel>
#include <KIcon>
#include <KLocale>
#include <KStandardDirs>

// Own includes
#include <Category/Category.h>
#include "ApplicationBackend.h"
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "CategoryView/CategoryViewWidget.h"

AvailableView::AvailableView(QWidget *parent, ApplicationBackend *backend)
        : AbstractViewContainer(parent)
        , m_backend(backend)
{

    m_categoryViewWidget = new CategoryViewWidget(m_viewStack, m_backend);

    QString rootName = i18n("Get Software");
    KIcon rootIcon = KIcon("applications-other");
    m_categoryViewWidget->setCategories(Category::populateCategories(), rootName, rootIcon);
    m_breadcrumbWidget->setRootItem(m_categoryViewWidget->breadcrumbItem());

    m_viewStack->addWidget(m_categoryViewWidget);
    m_viewStack->setCurrentWidget(m_categoryViewWidget);

    connect(m_backend, SIGNAL(xapianReloaded()),
            m_breadcrumbWidget, SLOT(startSearch()));
    connect(m_categoryViewWidget, SIGNAL(registerNewSubView(AbstractViewBase*)),
            this, SLOT(registerNewSubView(AbstractViewBase*)));
    connect(m_categoryViewWidget, SIGNAL(switchToSubView(AbstractViewBase*)),
            this, SLOT(switchToSubView(AbstractViewBase*)));
}
