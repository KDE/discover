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
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtGui/QSplitter>

// KDE includes
#include <KDebug>
#include <KHBox>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuonapt/MuonStrings.h"
#include "DetailsWidget.h"
#include "PackageModel/PackageModel.h"
#include "PackageModel/PackageProxyModel.h"
#include "PackageModel/PackageView.h"
#include "PackageModel/PackageDelegate.h"

ManagerWidget::ManagerWidget(QWidget *parent)
    : PackageWidget(parent)
{
    setPackagesType(PackageWidget::AvailablePackages);

    hideHeaderLabel();
    showSearchEdit();
}

void ManagerWidget::reload()
{
    PackageWidget::reload();
    startSearch();
}

void ManagerWidget::filterByGroup(const QString &groupName)
{
    QString groupKey = MuonStrings::global()->groupKey(groupName);
    if (groupName == i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All")) {
        groupKey.clear();
    }
    m_proxyModel->setGroupFilter(groupKey);
}

void ManagerWidget::filterByStatus(const QApt::Package::State state)
{
    m_proxyModel->setStateFilter(state);
}

void ManagerWidget::filterByOrigin(const QString &originName)
{
    QString origin = m_backend->origin(originName);
    m_proxyModel->setOriginFilter(origin);
}

void ManagerWidget::filterByArchitecture(const QString &arch)
{
    m_proxyModel->setArchFilter(arch);
}

#include "ManagerWidget.moc"
