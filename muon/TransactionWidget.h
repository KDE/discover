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

#ifndef TRANSACTIONWIDGET_H
#define TRANSACTIONWIDGET_H

#include <QtGui/QWidget>

#include <LibQApt/Globals>

class QLabel;
class QProgressBar;
class QPushButton;
class QTreeView;

namespace QApt {
    class Transaction;
}

namespace DebconfKde
{
    class DebconfGui;
}

class DownloadModel;
class DownloadDelegate;

class TransactionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TransactionWidget(QWidget *parent = 0);

    void setTransaction(QApt::Transaction *trans);
    
private:
    QApt::Transaction *m_trans;
    int m_lastRealProgress;

    QLabel *m_headerLabel;
    QTreeView *m_downloadView;
    DownloadModel *m_downloadModel;
    DownloadDelegate *m_downloadDelegate;
    DebconfKde::DebconfGui *m_debconfGui;
    QProgressBar *m_totalProgress;
    QLabel *m_statusLabel;
    QPushButton *m_cancelButton;

private slots:
    void statusChanged(QApt::TransactionStatus status);
    void transactionErrorOccurred(QApt::ErrorCode error);
    void provideMedium(const QString &medium, const QString &label);
    void untrustedPrompt(const QStringList &untrustedPackages);
    void updateProgress(int progress);
};

#endif // TRANSACTIONWIDGET_H
