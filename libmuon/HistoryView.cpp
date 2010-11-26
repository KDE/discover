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

#include <QtGui/QListView>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QStandardItemModel>

#include <KGlobal>
#include <KIcon>
#include <KLocale>
#include <KDebug>

#include <LibQApt/History>

HistoryView::HistoryView(QWidget *parent)
    : KVBox(parent)
{
    m_history = new QApt::History(this);

    m_historyModel = new QStandardItemModel(this);
    m_historyView = new QTreeView(this);

    QDateTime currentDateTime = QDateTime::currentDateTime();
    foreach (QApt::HistoryItem *item, m_history->historyItems()) {
        QString category;
        QDateTime startDateTime = item->startDate();

        currentDateTime.addDays(-7);
        if (currentDateTime < startDateTime) {
            //category = KGlobal::locale()->dayPeriodText(startDateTime.time(), KLocale::LongName);
            category = startDateTime.toString("dddd");
        } else {
            category = startDateTime.toString("MMMM dd");
        }

        QStandardItem *parentItem = 0;

        if (!m_categoryHash.contains(category)) {
            parentItem = new QStandardItem;
            parentItem->setEditable(false);
            parentItem->setText(category);

            m_historyModel->appendRow(parentItem);
            m_categoryHash[category] = parentItem;
        } else {
            parentItem = m_categoryHash.value(category);
        }

        foreach (const QString &package, item->packageList()) {
            QStandardItem *historyItem = new QStandardItem;
            historyItem->setEditable(false);
            historyItem->setIcon(KIcon("applications-other").pixmap(32,32));
            QString formattedTime = KGlobal::locale()->formatTime(startDateTime.time());

            QString action;
            switch(item->action()) {
            case QApt::Package::ToInstall:
                action = i18nc("@status describes a past-tense action", "Installed");
                break;
            case QApt::Package::ToUpgrade:
                action = i18nc("@status describes a past-tense action", "Upgraded");
                break;
            case QApt::Package::ToDowngrade:
                action = i18nc("@status describes a past-tense action", "Downgraded");
                break;
            case QApt::Package::ToRemove:
                action = i18nc("@status describes a past-tense action", "Removed");
                break;
            case QApt::Package::ToPurge:
                action = i18nc("@status describes a past-tense action", "Purged");
            default:
                break;
            }

            QString text = i18nc("@item example: muon installed at 16:00", "%1 %2 at %3",
                                 package, action, formattedTime);
            historyItem->setText(text);

            parentItem->appendRow(historyItem);
        }
    }

    m_historyView->setMouseTracking(true);
    m_historyView->setVerticalScrollMode(QListView::ScrollPerPixel);

    m_historyView->setModel(m_historyModel);
}

HistoryView::~HistoryView()
{
}

#include "HistoryView.moc"
