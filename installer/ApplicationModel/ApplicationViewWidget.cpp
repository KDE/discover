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

#include "ApplicationViewWidget.h"

#include <QApplication>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KDebug>

#include <LibQApt/Backend>

#include "ApplicationModel.h"
#include "ApplicationProxyModel.h"
#include "ApplicationDelegate.h"
#include "../Application.h"
#include "../ApplicationBackend.h"
#include "../ApplicationDetailsView/ApplicationDetailsView.h"
#include "../BreadcrumbWidget/BreadcrumbItem.h"
#include "../CategoryView/Category.h"

ApplicationViewWidget::ApplicationViewWidget(QWidget *parent, ApplicationBackend *appBackend)
        : AbstractViewBase(parent)
        , m_backend(0)
        , m_appBackend(appBackend)
        , m_detailsView(0)
{
    m_appModel = new ApplicationModel(this);
    m_proxyModel = new ApplicationProxyModel(this);
    m_proxyModel->setSourceModel(m_appModel);

    m_treeView = new QTreeView(this);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);

    m_treeView->setModel(m_proxyModel);
    ApplicationDelegate *delegate = new ApplicationDelegate(m_treeView);
    m_treeView->setItemDelegate(delegate);

    m_layout->addWidget(m_treeView);

    connect(delegate, SIGNAL(infoButtonClicked(Application *)),
            this, SLOT(infoButtonClicked(Application *)));
    connect(delegate, SIGNAL(installButtonClicked(Application *)),
            this, SIGNAL(installButtonClicked(Application *)));
    connect(delegate, SIGNAL(removeButtonClicked(Application *)),
            this, SIGNAL(removeButtonClicked(Application *)));
}

ApplicationViewWidget::~ApplicationViewWidget()
{
}

void ApplicationViewWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_appModel->setMaxPopcon(m_appBackend->maxPopconScore());
    m_appModel->setApplications(m_appBackend->applicationList());
    m_proxyModel->setBackend(backend);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    m_crumb->setAssociatedView(this);
}

void ApplicationViewWidget::reload()
{
    m_appModel->clear();
    m_proxyModel->invalidate();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);

    m_appModel->setMaxPopcon(m_appBackend->maxPopconScore());
    m_appModel->setApplications(m_appBackend->applicationList());

    m_proxyModel->setSourceModel(m_appModel);
    m_proxyModel->parentDataChanged();
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
}

void ApplicationViewWidget::setTitle(const QString &title)
{
    m_crumb->setText(title);
}

void ApplicationViewWidget::setIcon(const QIcon &icon)
{
    m_crumb->setIcon(icon);
}

void ApplicationViewWidget::setStateFilter(QApt::Package::State state)
{
    m_proxyModel->setStateFilter(state);
}

void ApplicationViewWidget::setOriginFilter(const QString &origin)
{
    m_proxyModel->setOriginFilter(origin);
}

void ApplicationViewWidget::setFiltersFromCategory(Category *category)
{
    m_proxyModel->setFiltersFromCategory(category);
}

void ApplicationViewWidget::infoButtonClicked(Application *app)
{
    // Check to see if a view for this app already exists
    if (m_currentPair.second == app) {
        emit switchToSubView(m_currentPair.first);
        return;
    }

    // Create one if not
    m_detailsView = new ApplicationDetailsView(this, app);
    m_currentPair.first = m_detailsView;

    connect(m_detailsView, SIGNAL(destroyed(QObject *)),
            this, SLOT(onSubViewDestroyed()));

    // Tell our parent that we can exist, so that they can forward it
    emit registerNewSubView(m_detailsView);
}

void ApplicationViewWidget::onSubViewDestroyed()
{
    m_currentPair.first = 0;
    m_currentPair.second = 0;
}

#include "ApplicationViewWidget.moc"
