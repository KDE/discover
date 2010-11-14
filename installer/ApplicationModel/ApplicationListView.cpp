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

#include "ApplicationListView.h"

// Qt includes
#include <QtGui/QIcon>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KSeparator>

// Own includes
#include "ApplicationViewWidget.h"
#include "../AbstractViewBase.h"
#include "../BreadcrumbWidget/BreadcrumbItem.h"
#include "../BreadcrumbWidget/BreadcrumbWidget.h"

ApplicationListView::ApplicationListView(QWidget *parent, ApplicationBackend *appBackend,
                                         const QModelIndex &index)
        : QWidget(parent)
        , m_backend(0)
        , m_appBackend(appBackend)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    m_viewStack = new QStackedWidget(this);

    m_appViewWidget = new ApplicationViewWidget(this, appBackend);
    m_appViewWidget->setTitle(index.data(Qt::DisplayRole).toString());
    m_appViewWidget->setIcon(index.data(Qt::DecorationRole).value<QIcon>());

    m_breadcrumbWidget = new BreadcrumbWidget(this);
    m_breadcrumbWidget->setRootItem(m_appViewWidget->breadcrumbItem());

    KSeparator *horizonatalSeparator = new KSeparator(this);
    horizonatalSeparator->setOrientation(Qt::Horizontal);

    m_viewStack->addWidget(m_appViewWidget);
    m_viewStack->setCurrentWidget(m_appViewWidget);

    layout->addWidget(m_breadcrumbWidget);
    layout->addWidget(horizonatalSeparator);
    layout->addWidget(m_viewStack);

    connect(m_breadcrumbWidget, SIGNAL(itemActivated(BreadcrumbItem *)),
            this, SLOT(activateBreadcrumbItem(BreadcrumbItem *)));
    connect(m_appViewWidget, SIGNAL(registerNewSubView(AbstractViewBase *)),
            this, SLOT(registerNewSubView(AbstractViewBase *)));
    connect(m_appViewWidget, SIGNAL(switchToSubView(AbstractViewBase *)),
            this, SLOT(switchToSubView(AbstractViewBase *)));
}

ApplicationListView::~ApplicationListView()
{
}

void ApplicationListView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;

    m_appViewWidget->setBackend(backend);
}

void ApplicationListView::setStateFilter(QApt::Package::State state)
{
    m_appViewWidget->setStateFilter(state);
}

void ApplicationListView::setOriginFilter(const QString &origin)
{
    m_appViewWidget->setOriginFilter(origin);
}

// FIXME: Next 3 functions can be copied verbatim to AvailableView
// ApplicationListView and AppListView need a common base
void ApplicationListView::registerNewSubView(AbstractViewBase *subView)
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

void ApplicationListView::switchToSubView(AbstractViewBase *subView)
{
    m_viewStack->setCurrentWidget(subView);
    m_breadcrumbWidget->setItemBolded(m_breadcrumbWidget->breadcrumbForView(subView));
}

void ApplicationListView::activateBreadcrumbItem(BreadcrumbItem *item)
{
    AbstractViewBase *toActivate = item->associatedView();
    if (!toActivate) {
        // Screwed
        return;
    }

    m_viewStack->setCurrentWidget(toActivate);
    m_breadcrumbWidget->setItemBolded(item);
}

#include "ApplicationListView.moc"
