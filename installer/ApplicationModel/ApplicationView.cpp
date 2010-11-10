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

#include "ApplicationView.h"

#include <QApplication>
#include <QtGui/QTreeView>

#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KDebug>

#include <LibQApt/Backend>

#include "ApplicationModel.h"
#include "ApplicationProxyModel.h"
#include "ApplicationDelegate.h"
#include "../ApplicationWindow.h"

ApplicationView::ApplicationView(ApplicationWindow *parent)
        : KVBox(parent)
        , m_parent(parent)
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
}

ApplicationView::~ApplicationView()
{
}

void ApplicationView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_appModel->setMaxPopcon(m_parent->maxPopconScore());
    m_appModel->setApplications(m_parent->applicationList());
    m_proxyModel->setBackend(backend);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
}

void ApplicationView::reload()
{
    m_appModel->clear();
    m_proxyModel->invalidate();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);

    m_appModel->setMaxPopcon(m_parent->maxPopconScore());
    m_appModel->setApplications(m_parent->applicationList());

    m_proxyModel->setSourceModel(m_appModel);
    m_proxyModel->parentDataChanged();
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
}

void ApplicationView::setStateFilter(QApt::Package::State state)
{
    m_proxyModel->setStateFilter(state);
}

void ApplicationView::setOriginFilter(const QString &origin)
{
    m_proxyModel->setOriginFilter(origin);
}

#include "ApplicationView.moc"
