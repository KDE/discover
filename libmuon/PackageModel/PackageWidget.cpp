/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2010 Guillaume Martres <smarter@ubuntu.com>               *
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

#include "PackageWidget.h"

// Qt includes
#include <QtGui/QHeaderView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>

// KDE includes
#include <KIcon>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/DetailsWidget.h"
#include "../libmuon/PackageModel/PackageModel.h"
#include "../libmuon/PackageModel/PackageProxyModel.h"
#include "../libmuon/PackageModel/PackageView.h"
#include "../libmuon/PackageModel/PackageDelegate.h"

PackageWidget::PackageWidget(QWidget *parent)
: QSplitter(parent)
, m_backend(0), m_headerWidget(0), m_packagesType(0)
{
    setOrientation(Qt::Vertical);

    m_model = new PackageModel(this);
    PackageDelegate *delegate = new PackageDelegate(this);
    m_proxyModel = new PackageProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    QWidget *topWidget = new QWidget(this);
    setStretchFactor(0, 4);
    m_topLayout = new QVBoxLayout(topWidget);

    m_packageView = new PackageView(topWidget);
    m_packageView->setModel(m_proxyModel);
    m_packageView->setItemDelegate(delegate);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);

    m_topLayout->addWidget(m_packageView);

    m_detailsWidget = new DetailsWidget(this);

    setEnabled(false);

    connect(m_packageView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(packageActivated(const QModelIndex &)));
    connect(m_packageView, SIGNAL(currentPackageChanged(const QModelIndex &)),
            this, SLOT(packageActivated(const QModelIndex &)));
}

PackageWidget::~PackageWidget()
{
}

void PackageWidget::setHeaderWidget(QWidget* widget)
{
    if (m_topLayout->count() == 2) {
        m_topLayout->takeAt(0);
    }
    m_topLayout->insertWidget(0, widget);
}

void PackageWidget::setPackagesType(int type)
{
    m_packagesType = type;

    switch (m_packagesType) {
        case AvailablePackages:
            break;
        case UpgradeablePackages: {
            QApt::Package::State state =
                        (QApt::Package::State)(QApt::Package::Upgradeable | QApt::Package::ToInstall |
                        QApt::Package::ToReInstall | QApt::Package::ToUpgrade |
                        QApt::Package::ToDowngrade | QApt::Package::ToRemove |
                        QApt::Package::ToPurge);
            m_proxyModel->setStateFilter(state);
            break;
        }
        case MarkedPackages: {
            QApt::Package::State state =
                          (QApt::Package::State)(QApt::Package::ToInstall |
                          QApt::Package::ToReInstall | QApt::Package::ToUpgrade |
                          QApt::Package::ToDowngrade | QApt::Package::ToRemove |
                          QApt::Package::ToPurge);
            m_proxyModel->setStateFilter(state);
            break;
        }
    }
}

void PackageWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), m_detailsWidget, SLOT(refreshTabs()));

    m_detailsWidget->setBackend(backend);
    m_proxyModel->setBackend(m_backend);
    m_model->setPackages(m_backend->availablePackages());
    m_packageView->updateView();
    m_packageView->setSortingEnabled(true);

    setEnabled(true);
}

void PackageWidget::reload()
{
    m_detailsWidget->clear();
    m_model->clear();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);
    m_model->setPackages(m_backend->availablePackages());
    m_proxyModel->setSourceModel(m_model);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_packageView->sortByColumn(0, Qt::DescendingOrder);
}

void PackageWidget::packageActivated(const QModelIndex &index)
{
    QApt::Package *package = m_proxyModel->packageAt(index);
    if (package == 0) {
        m_detailsWidget->hide();
        return;
    }
    m_detailsWidget->setPackage(package);
}

#include "PackageWidget.moc"
