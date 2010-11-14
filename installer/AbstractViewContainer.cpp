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

#include "AbstractViewContainer.h"

// Qt includes
#include <QtGui/QStackedWidget>

// KDE includes
#include <KSeparator>

#include "AbstractViewBase.h"
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "BreadcrumbWidget/BreadcrumbItem.h"

AbstractViewContainer::AbstractViewContainer(QWidget *parent)
        : KVBox(parent)
{
    setSpacing(2);
    m_breadcrumbWidget = new BreadcrumbWidget(this);

    KSeparator *horizontalSeparator = new KSeparator(this);
    horizontalSeparator->setOrientation(Qt::Horizontal);

    m_viewStack = new QStackedWidget(this);

    connect(m_breadcrumbWidget, SIGNAL(itemActivated(BreadcrumbItem *)),
            this, SLOT(activateBreadcrumbItem(BreadcrumbItem *)));
}

AbstractViewContainer::~AbstractViewContainer()
{
}

void AbstractViewContainer::registerNewSubView(AbstractViewBase *subView)
{
    AbstractViewBase *currentView = static_cast<AbstractViewBase *>(m_viewStack->currentWidget());
    BreadcrumbItem *currentItem = m_breadcrumbWidget->breadcrumbForView(currentView);

    // If we are activating a new subView from a view that already has
    // children, the old ones must go
    if (currentItem->hasChildren()) {
        m_breadcrumbWidget->removeItem(currentItem->childItem());
    }

    m_viewStack->addWidget(subView);
    m_viewStack->setCurrentWidget(subView);
    m_breadcrumbWidget->addLevel(subView->breadcrumbItem());
}

void AbstractViewContainer::switchToSubView(AbstractViewBase *subView)
{
    m_viewStack->setCurrentWidget(subView);
    m_breadcrumbWidget->setItemBolded(m_breadcrumbWidget->breadcrumbForView(subView));
}

void AbstractViewContainer::activateBreadcrumbItem(BreadcrumbItem *item)
{
    AbstractViewBase *toActivate = item->associatedView();
    if (!toActivate) {
        // Screwed
        return;
    }

    m_viewStack->setCurrentWidget(toActivate);
    m_breadcrumbWidget->setItemBolded(item);
}

#include "AbstractViewContainer.moc"
