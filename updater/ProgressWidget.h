/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef PROGRESSWIDGET_H
#define PROGRESSWIDGET_H

#include <QtGui/QWidget>

#include <LibQApt/Globals>

class QLabel;
class QParallelAnimationGroup;
class QPushButton;
class QProgressBar;

namespace QApt {
    class Transaction;
}

class ProgressWidget : public QWidget
{
    Q_OBJECT
public:
    ProgressWidget(QWidget *parent);

    void setTransaction(QApt::Transaction *trans);

private:
    QApt::Transaction *m_trans;
    int m_lastRealProgress;

    QLabel *m_headerLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_cancelButton;
    QLabel *m_detailsLabel;
    bool m_show;

    QParallelAnimationGroup *m_expandWidget;

public Q_SLOTS:
    void show();
    void animatedHide();

private Q_SLOTS:
    void statusChanged(QApt::TransactionStatus status);
    void transactionErrorOccurred(QApt::ErrorCode error);
    void provideMedium(const QString &label, const QString &medium);
    void untrustedPrompt(const QStringList &untrustedPackages);
    void configFileConflict(const QString &currentPath, const QString &newPath);
    void updateProgress(int progress);
    void downloadSpeedChanged(quint64 speed);
    void etaChanged(quint64 ETA);
};

#endif // PROGRESSWIDGET_H
