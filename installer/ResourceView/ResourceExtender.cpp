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

#include "ResourceExtender.h"

#include <QHBoxLayout>
#include <QtWidgets/QPushButton>

#include <KDebug>
#include <KLocalizedString>
#include <KStandardGuiItem>

#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include "Transaction/TransactionModel.h"

ResourceExtender::ResourceExtender(QWidget *parent, AbstractResource *app)
    : QWidget(parent)
    , m_resource(app)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    m_infoButton = new QPushButton(this);
    m_infoButton->setText(i18n("More Info"));
    connect(m_infoButton, SIGNAL(clicked()), this, SLOT(emitInfoButtonClicked()));

    QWidget *buttonSpacer = new QWidget(this);
    buttonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_actionButton = new QPushButton(this);

    if (app->isInstalled()) {
        m_actionButton->setIcon(QIcon::fromTheme("edit-delete"));
        m_actionButton->setText(i18n("Remove"));
        connect(m_actionButton, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));
    } else {
        m_actionButton->setIcon(QIcon::fromTheme("download"));
        m_actionButton->setText(i18n("Install"));
        connect(m_actionButton, SIGNAL(clicked()), this, SLOT(installButtonClicked()));
    }

    m_cancelButton = new QPushButton(this);
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    m_cancelButton->setIcon(cancelButton.icon());
    m_cancelButton->setToolTip(cancelButton.toolTip());
    m_cancelButton->hide();
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));

    layout->addWidget(m_infoButton);
    layout->addWidget(buttonSpacer);
    layout->addWidget(m_actionButton);
    layout->addWidget(m_cancelButton);

    // Catch already-begun transactions
    TransactionModel *transModel = TransactionModel::global();
    setupTransaction(transModel->transactionFromResource(app));

    connect(transModel, SIGNAL(transactionAdded(Transaction*)),
            this, SLOT(setupTransaction(Transaction*)));
    connect(transModel, SIGNAL(transactionCancelled(Transaction*)),
            this, SLOT(transactionCancelled(Transaction*)));
}

void ResourceExtender::setShowInfoButton(bool show)
{
    m_infoButton->setVisible(show);
}

void ResourceExtender::setupTransaction(Transaction *trans)
{
    if (!trans || m_resource != trans->resource())
        return;

    // Transaction already started, so no action can be taken
    m_actionButton->hide();
    m_cancelButton->setVisible(trans->isCancellable());

    // Listen to future changes with the transaction
    connect(trans, SIGNAL(cancellableChanged(bool)),
            m_cancelButton, SLOT(setVisible(bool)));
}

void ResourceExtender::transactionCancelled(Transaction* trans)
{
    AbstractResource* resource = trans->resource();
    if (m_resource != resource)
        return;

    // Reset UI to its pre-transaction state
    m_cancelButton->hide();
    m_actionButton->show();
    m_actionButton->setEnabled(true);
    if (m_resource->isInstalled()) {
        m_actionButton->setIcon(QIcon::fromTheme("edit-delete"));
        m_actionButton->setText(i18n("Remove"));
    } else {
        m_actionButton->setIcon(QIcon::fromTheme("download"));
        m_actionButton->setText(i18n("Install"));
    }
}

void ResourceExtender::emitInfoButtonClicked()
{
    emit infoButtonClicked(m_resource);
}

void ResourceExtender::removeButtonClicked()
{
    m_actionButton->setEnabled(false);
    ResourcesModel *resourcesModel = ResourcesModel::global();
    resourcesModel->removeApplication(m_resource);
}

void ResourceExtender::installButtonClicked()
{
    m_actionButton->setEnabled(false);
    ResourcesModel *resourcesModel = ResourcesModel::global();
    resourcesModel->installApplication(m_resource);
}

void ResourceExtender::cancelButtonClicked()
{
    ResourcesModel *resourcesModel = ResourcesModel::global();
    resourcesModel->cancelTransaction(m_resource);
}
