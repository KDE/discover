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
#include <KDebug>

#include <resources/AbstractResourcesBackend.h>
#include <Transaction/TransactionModel.h>

#include "ApplicationView/ApplicationDelegate.h"

ProgressView::ProgressView(QWidget *parent, AbstractResourcesBackend *backend)
    : KVBox(parent)
{
    QLabel *headerLabel = new QLabel(this);
    headerLabel->setText(i18nc("@info", "<title>In Progress</title>"));
    headerLabel->setAlignment(Qt::AlignLeft);

    m_progressModel = new TransactionModel(this);
    m_progressModel->setBackend(backend);
    m_progressModel->addTransactions(backend->transactions());

    QListView *listView = new QListView(this);
    listView->setAlternatingRowColors(true);
    ApplicationDelegate *delegate = new ApplicationDelegate(listView, backend);
    delegate->setShowInfoButton(false);
    listView->setItemDelegate(delegate);
    listView->setModel(m_progressModel);

    connect(delegate, SIGNAL(cancelButtonClicked(AbstractResource*)),
            backend, SLOT(cancelTransaction(AbstractResource*)));
    connect(m_progressModel, SIGNAL(lastTransactionCancelled()),
            this, SIGNAL(lastTransactionCancelled()));
}
