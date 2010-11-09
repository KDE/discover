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

#include "FilterWidget.h"

// Qt includes
#include <QStandardItemModel>
#include <QtCore/QSet>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QTreeView>
#include <QtGui/QToolBox>

// KDE includes
#include <KIcon>
#include <KLineEdit>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/MuonStrings.h"

FilterWidget::FilterWidget(QWidget *parent)
    : QDockWidget(parent)
    , m_backend(0)
{
    m_strings = new MuonStrings(this);
    setFeatures(QDockWidget::NoDockWidgetFeatures);
    setWindowTitle(i18nc("@title:window", "Filter:"));

    m_filterBox = new QToolBox(this);
    m_filterBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_categoriesList = new QTreeView(this);
    m_categoriesList->setAlternatingRowColors(true);
    m_categoriesList->setRootIsDecorated(false);
    m_categoriesList->setHeaderHidden(true);
    m_filterBox->addItem(m_categoriesList, KIcon(), i18nc("@title:tab", "By Category"));
    m_categoryModel = new QStandardItemModel;
    connect(m_categoriesList, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(categoryActivated(const QModelIndex &)));

    m_statusList = new QListView(this);
    m_statusList->setAlternatingRowColors(true);
    m_filterBox->addItem(m_statusList, KIcon(), i18nc("@title:tab", "By Status"));
    m_statusModel = new QStandardItemModel;
    connect(m_statusList, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(statusActivated(const QModelIndex &)));

    m_originList = new QListView(this);
    m_originList->setAlternatingRowColors(true);
    m_filterBox->addItem(m_originList, KIcon(), i18nc("@title:tab", "By Origin"));
    m_originModel = new QStandardItemModel;
    connect(m_originList, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(originActivated(const QModelIndex &)));

    setWidget(m_filterBox);
    setEnabled(false);
}

FilterWidget::~FilterWidget()
{
}

void FilterWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;

    populateCategories();
    m_categoriesList->setModel(m_categoryModel);
    selectFirstRow(m_categoriesList);

    populateStatuses();
    m_statusList->setModel(m_statusModel);
    selectFirstRow(m_statusList);

    populateOrigins();
    m_originList->setModel(m_originModel);
    selectFirstRow(m_originList);

    setEnabled(true);
}

void FilterWidget::populateCategories()
{
    QApt::GroupList groups = m_backend->availableGroups();
    QSet<QString> groupSet;

    foreach(const QApt::Group &group, groups) {
        QString groupName = m_strings->groupName(group);

        if (!groupName.isEmpty()) {
            groupSet << groupName;
        }
    }

    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));
    m_categoryModel->appendRow(defaultItem);

    QStringList groupList = groupSet.toList();
    qSort(groupList);

    foreach(const QString &group, groupList) {
        QStandardItem *groupItem = new QStandardItem;
        groupItem->setEditable(false);
        groupItem->setText(group);
        m_categoryModel->appendRow(groupItem);
    }
}

void FilterWidget::populateStatuses()
{
    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));

    QStandardItem *installedItem = new QStandardItem;
    installedItem->setEditable(false);
    installedItem->setIcon(KIcon("download"));
    installedItem->setText(m_strings->packageStateName(QApt::Package::Installed));
    installedItem->setData(QApt::Package::Installed);

    QStandardItem *notInstalledItem = new QStandardItem;
    notInstalledItem->setEditable(false);
    notInstalledItem->setIcon(KIcon("application-x-deb"));
    notInstalledItem->setText(m_strings->packageStateName(QApt::Package::NotInstalled));
    notInstalledItem->setData(QApt::Package::NotInstalled);

    QStandardItem *upgradeableItem = new QStandardItem;
    upgradeableItem->setEditable(false);
    upgradeableItem->setIcon(KIcon("system-software-update"));
    upgradeableItem->setText(m_strings->packageStateName(QApt::Package::Upgradeable));
    upgradeableItem->setData(QApt::Package::Upgradeable);

    QStandardItem *brokenItem = new QStandardItem;
    brokenItem->setEditable(false);
    brokenItem->setIcon(KIcon("dialog-cancel"));
    brokenItem->setText(m_strings->packageStateName(QApt::Package::NowBroken));
    brokenItem->setData(QApt::Package::NowBroken);

    QStandardItem *purgeableItem = new QStandardItem;
    purgeableItem->setEditable(false);
    purgeableItem->setIcon(KIcon("user-trash-full"));
    purgeableItem->setText(m_strings->packageStateName(QApt::Package::ResidualConfig));
    purgeableItem->setData(QApt::Package::ResidualConfig);

    QStandardItem *autoRemoveItem = new QStandardItem;
    autoRemoveItem->setEditable(false);
    autoRemoveItem->setIcon(KIcon("archive-remove"));
    autoRemoveItem->setText(m_strings->packageStateName(QApt::Package::IsGarbage));
    autoRemoveItem->setData(QApt::Package::IsGarbage);

    m_statusModel->appendRow(defaultItem);
    m_statusModel->appendRow(installedItem);
    m_statusModel->appendRow(notInstalledItem);
    m_statusModel->appendRow(upgradeableItem);
    m_statusModel->appendRow(brokenItem);
    m_statusModel->appendRow(purgeableItem);
    m_statusModel->appendRow(autoRemoveItem);
}

void FilterWidget::populateOrigins()
{
    QStringList originLabels = m_backend->originLabels();

    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));
    m_originModel->appendRow(defaultItem);

    foreach(const QString &originLabel, originLabels) {
        QStandardItem *originItem = new QStandardItem;
        originItem->setEditable(false);
        originItem->setText(originLabel);
        m_originModel->appendRow(originItem);
    }
}

void FilterWidget::categoryActivated(const QModelIndex &index)
{
    QString groupName = index.data(Qt::DisplayRole).toString();
    emit filterByGroup(groupName);
}

void FilterWidget::statusActivated(const QModelIndex &index)
{
    QApt::Package::State state = (QApt::Package::State)index.data(Qt::UserRole+1).toInt();
    emit filterByStatus(state);
}

void FilterWidget::originActivated(const QModelIndex &index)
{
    QString originName = index.data(Qt::DisplayRole).toString();
    emit filterByOrigin(originName);
}

void FilterWidget::selectFirstRow(const QAbstractItemView *itemView)
{
    QModelIndex firstRow = itemView->model()->index(0, 0);
    itemView->selectionModel()->select(firstRow, QItemSelectionModel::Select);
}

#include "FilterWidget.moc"
