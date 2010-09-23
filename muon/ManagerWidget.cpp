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

#include "ManagerWidget.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QHeaderView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSplitter>

// KDE includes
#include <KDebug>
#include <KHBox>
#include <KIcon>
#include <KLineEdit>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/DetailsWidget.h"
#include "../libmuon/MuonStrings.h"
#include "../libmuon/PackageModel/PackageModel.h"
#include "../libmuon/PackageModel/PackageProxyModel.h"
#include "../libmuon/PackageModel/PackageView.h"
#include "../libmuon/PackageModel/PackageDelegate.h"

ManagerWidget::ManagerWidget(QWidget *parent)
    : PackageWidget(parent)
{
    setPackagesType(PackageWidget::AvailablePackages);

    m_strings = new MuonStrings(this);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(300);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));

    m_searchEdit = new KLineEdit(this);
    m_searchEdit->setClickMessage(i18nc("@label Line edit click message", "Search"));
    m_searchEdit->setClearButtonShown(true);
    setHeaderWidget(m_searchEdit);
    connect(m_searchEdit, SIGNAL(textChanged(const QString &)), m_searchTimer, SLOT(start()));
}

ManagerWidget::~ManagerWidget()
{
}

void ManagerWidget::setFocus()
{
    m_searchEdit->setFocus();
}

void ManagerWidget::reload()
{
    m_detailsWidget->clear();
    m_model->clear();
    m_proxyModel->invalidate();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);
    m_backend->reloadCache();
    m_model->setPackages(m_backend->availablePackages());
    m_proxyModel->setSourceModel(m_model);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_packageView->sortByColumn(0, Qt::DescendingOrder);
    startSearch();
}

void ManagerWidget::startSearch()
{
    m_proxyModel->search(m_searchEdit->text());
}

void ManagerWidget::filterByGroup(const QString &groupName)
{
    QString groupKey = m_strings->groupKey(groupName);
    if (groupName == i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All")) {
        groupKey.clear();
    }
    m_proxyModel->setGroupFilter(groupKey);
}

void ManagerWidget::filterByStatus(const QString &statusName)
{
    QApt::Package::State state = m_strings->packageStateKey(statusName);
    m_proxyModel->setStateFilter(state);
}

void ManagerWidget::filterByOrigin(const QString &originName)
{
    QString origin = m_backend->origin(originName);
    m_proxyModel->setOriginFilter(origin);
}

#include "ManagerWidget.moc"
