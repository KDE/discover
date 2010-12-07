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

#ifndef APPLICATIONEXTENDER_H
#define APPLICATIONEXTENDER_H

#include <QtGui/QWidget>

#include <LibQApt/Globals>

#include "ApplicationBackend.h"
#include "Transaction.h"

class QProgressBar;
class QPushButton;

class Application;
class ApplicationBackend;

class ApplicationExtender : public QWidget
{
    Q_OBJECT
public:
    ApplicationExtender(QWidget *parent, Application *app, ApplicationBackend *backend);
    ~ApplicationExtender();

private:
    Application *m_app;
    ApplicationBackend *m_appBackend;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    QPushButton *m_cancelButton;

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event, Application *app);
    void updateProgress(Application *app, int percentage);
    void showTransactionState(TransactionState state);
    void transactionCancelled(Application *app);
    void emitInfoButtonClicked();
    void emitRemoveButtonClicked();
    void emitInstallButtonClicked();
    void emitCancelButtonClicked();

Q_SIGNALS:
    void infoButtonClicked(Application *app);
    void removeButtonClicked(Application *app);
    void installButtonClicked(Application *app);
    void cancelButtonClicked(Application *app);
};

#endif
