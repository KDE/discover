/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ProgressView.h"

#include <QtGui/QLabel>
#include <QtGui/QListView>

#include <KLocalizedString>

#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>

#include "ResourceView/ResourceDelegate.h"

ProgressView::ProgressView(QWidget *parent)
    : KVBox(parent)
{
    QLabel *headerLabel = new QLabel(this);
    headerLabel->setText(i18nc("@info", "<title>In Progress</title>"));
    headerLabel->setAlignment(Qt::AlignLeft);

    ResourcesModel *model = ResourcesModel::global();
    ResourcesProxyModel *proxyModel = new ResourcesProxyModel(this);
    proxyModel->setFilterActive(true);
    proxyModel->setSourceModel(model);

    QListView *listView = new QListView(this);
    listView->setAlternatingRowColors(true);

    ResourceDelegate *delegate = new ResourceDelegate(listView);
    delegate->setShowInfoButton(false);
    connect(proxyModel, SIGNAL(invalidated()), delegate, SLOT(invalidate()));
    listView->setItemDelegate(delegate);
    listView->setModel(proxyModel);
}
