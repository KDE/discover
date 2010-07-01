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

#include "ReviewWidget.h"

// Qt includes
#include <QtGui/QHeaderView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KDebug>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "DetailsWidget.h"
#include "PackageModel/PackageModel.h"
#include "PackageModel/PackageProxyModel.h"
#include "PackageModel/PackageView.h"
#include "PackageModel/PackageDelegate.h"

ReviewWidget::ReviewWidget(QWidget *parent)
    : KVBox(parent)
    , m_backend(0)
{
    m_model = new PackageModel(this);
    PackageDelegate *delegate = new PackageDelegate(this);

    m_proxyModel = new PackageProxyModel(this);

    KVBox *topVBox = new KVBox;

    QLabel *browserHeader = new QLabel(topVBox);
    browserHeader->setTextFormat(Qt::RichText);
    browserHeader->setText(i18n("<b>Review and Apply Changes</b>"));

    m_packageView = new PackageView(topVBox);
    m_packageView->setModel(m_proxyModel);
    m_packageView->setItemDelegate(delegate);
    connect (m_packageView, SIGNAL(currentPackageChanged(const QModelIndex&)),
             this, SLOT(packageActivated(const QModelIndex&)));

    KVBox *bottomVBox = new KVBox;

    m_detailsWidget = new DetailsWidget(bottomVBox);

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(topVBox);
    splitter->addWidget(bottomVBox);

    setEnabled(false);
}

ReviewWidget::~ReviewWidget()
{
}

void ReviewWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(refresh()));
    connect(m_backend, SIGNAL(packageChanged()), m_detailsWidget, SLOT(refreshMainTabButtons()));

    m_model->setPackages(m_backend->markedPackages());
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setBackend(m_backend);
    m_packageView->setSortingEnabled(true);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);

    setEnabled(true);
}

void ReviewWidget::refresh()
{
    m_detailsWidget->clear();
    m_model->clear();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);
    m_model->setPackages(m_backend->markedPackages());
    m_proxyModel->setSourceModel(m_model);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_packageView->updateView();
}

void ReviewWidget::packageActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_detailsWidget->hide();
        return;
    }
    QApt::Package *package = m_proxyModel->packageAt(index);
    m_detailsWidget->setPackage(package);
}

#include "ReviewWidget.moc"
