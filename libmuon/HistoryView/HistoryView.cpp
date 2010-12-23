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

#include "HistoryView.h"

#include <QtCore/QTimer>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QStandardItemModel>

#include <KComboBox>
#include <KGlobal>
#include <KHBox>
#include <KIcon>
#include <KLineEdit>
#include <KLocale>
#include <KDebug>
#include <kdeversion.h>

#include <LibQApt/History>

#include "HistoryProxyModel.h"

HistoryView::HistoryView(QWidget *parent)
    : KVBox(parent)
{
    m_history = new QApt::History(this);

    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);

    QLabel *headerLabel = new QLabel(headerWidget);
    headerLabel->setText(i18nc("@info", "<title>History</title>"));

    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_searchEdit = new KLineEdit(headerWidget);
    m_searchEdit->setClickMessage(i18nc("@label Line edit click message", "Search"));
    m_searchEdit->setClearButtonShown(true);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(300);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));
    connect(m_searchEdit, SIGNAL(textChanged(const QString &)), m_searchTimer, SLOT(start()));

    m_filterBox = new KComboBox(headerWidget);
    m_filterBox->insertItem(AllChangesItem, KIcon("bookmark-new-list"),
                            i18nc("@item:inlistbox Filters all changes in the history view",
                                  "All changes"),
                            0);
    m_filterBox->insertItem(InstallationsItem, KIcon("download"),
                            i18nc("@item:inlistbox Filters installations in the history view",
                                  "Installations"),
                            QApt::Package::ToInstall);
    m_filterBox->insertItem(UpdatesItem, KIcon("system-software-update"),
                            i18nc("@item:inlistbox Filters updates in the history view",
                                  "Updates"),
                            QApt::Package::ToUpgrade);
    m_filterBox->insertItem(RemovalsItem, KIcon("edit-delete"),
                            i18nc("@item:inlistbox Filters removals in the history view",
                                  "Removals"),
                            (QApt::Package::State)(QApt::Package::ToRemove | QApt::Package::ToPurge));
    connect(m_filterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setStateFilter(int)));

    headerLayout->addWidget(headerLabel);
    headerLayout->addWidget(headerSpacer);
    headerLayout->addWidget(m_searchEdit);
    headerLayout->addWidget(m_filterBox);

    m_historyModel = new QStandardItemModel(this);
    m_historyModel->setColumnCount(1);
    m_historyModel->setHeaderData(0, Qt::Horizontal, i18nc("@title:column", "Date"));
    m_historyView = new QTreeView(this);

    QPixmap itemPixmap = KIcon("applications-other").pixmap(32,32);

    QDateTime weekAgoTime = QDateTime::currentDateTime().addDays(-7);
    foreach (QApt::HistoryItem *item, m_history->historyItems()) {
        QDateTime startDateTime = item->startDate();
        QString formattedTime = KGlobal::locale()->formatTime(startDateTime.time());
        QString category = KGlobal::locale()->formatDate(startDateTime.date(), KLocale::FancyShortDate);

        QStandardItem *parentItem = 0;

        if (!m_categoryHash.contains(category)) {
            parentItem = new QStandardItem;
            parentItem->setEditable(false);
            parentItem->setText(category);
            parentItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);

            m_historyModel->appendRow(parentItem);
            m_categoryHash[category] = parentItem;
        } else {
            parentItem = m_categoryHash.value(category);
        }

        foreach (const QString &package, item->installedPackages()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(itemPixmap);

            QString action = i18nc("@info:status describes a past-tense action", "Installed");
            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);
            historyItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);
            historyItem->setData(QApt::Package::ToInstall, HistoryProxyModel::HistoryActionRole);

            parentItem->appendRow(historyItem);
        }

        foreach (const QString &package, item->upgradedPackages()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(itemPixmap);

            QString action = i18nc("@status describes a past-tense action", "Upgraded");
            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);
            historyItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);
            historyItem->setData(QApt::Package::ToUpgrade, HistoryProxyModel::HistoryActionRole);

            parentItem->appendRow(historyItem);
        }

        foreach (const QString &package, item->downgradedPackages()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(itemPixmap);

            QString action = i18nc("@status describes a past-tense action", "Downgraded");
            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);
            historyItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);
            historyItem->setData(QApt::Package::ToDowngrade, HistoryProxyModel::HistoryActionRole);

            parentItem->appendRow(historyItem);
        }

        foreach (const QString &package, item->removedPackages()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(itemPixmap);

            QString action = i18nc("@status describes a past-tense action", "Removed");
            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);
            historyItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);
            historyItem->setData(QApt::Package::ToRemove, HistoryProxyModel::HistoryActionRole);

            parentItem->appendRow(historyItem);
        }

        foreach (const QString &package, item->purgedPackages()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(itemPixmap);

            QString action = i18nc("@status describes a past-tense action", "Purged");
            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);
            historyItem->setData(startDateTime, HistoryProxyModel::HistoryDateRole);
            historyItem->setData(QApt::Package::ToPurge, HistoryProxyModel::HistoryActionRole);

            parentItem->appendRow(historyItem);
        }
    }

    m_historyView->setMouseTracking(true);
    m_historyView->setVerticalScrollMode(QListView::ScrollPerPixel);

    m_proxyModel = new HistoryProxyModel(this);
    m_proxyModel->setSourceModel(m_historyModel);
    m_proxyModel->sort(0);

    m_historyView->setModel(m_proxyModel);
}

HistoryView::~HistoryView()
{
}

void HistoryView::setStateFilter(int index)
{
    QApt::Package::State state = (QApt::Package::State)m_filterBox->itemData(index).toInt();
    m_proxyModel->setStateFilter(state);
}

void HistoryView::startSearch()
{
    m_proxyModel->search(m_searchEdit->text());
}

#include "HistoryView.moc"
