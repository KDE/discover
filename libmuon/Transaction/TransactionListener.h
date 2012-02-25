/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

#ifndef TRANSACTIONLISTENER_H
#define TRANSACTIONLISTENER_H

#include <QObject>
#include <LibQApt/Globals>

#include "libmuonprivate_export.h"

class Transaction;
class Application;
class ApplicationBackend;

class MUONPRIVATE_EXPORT TransactionListener : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString comment READ comment NOTIFY commentChanged)
    Q_PROPERTY(Application* application READ application WRITE setApplication NOTIFY applicationChanged)
    Q_PROPERTY(ApplicationBackend* backend READ backend WRITE setBackend)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY running)
    public:
        explicit TransactionListener(QObject* parent = 0);
        virtual ~TransactionListener();
        void setBackend(ApplicationBackend* m_appBackend);
        void setApplication(Application* app);
        int progress() const;
        QString comment() const;
        Application* application() const;
        ApplicationBackend* backend() const;
        void init();
        bool isRunning() const;

    signals:
        void progressChanged();
        void commentChanged();
        void applicationChanged();
        void running(bool isRunning);
        void downloading(bool isDownloading);

    private slots:
        void workerEvent(QApt::WorkerEvent event, Transaction *transaction);
        void updateProgress(Transaction*,int);
        void transactionCancelled(Application*);

    private:
        void showTransactionState(Transaction* transaction);
        void setStateComment(Transaction* transaction);
        
        ApplicationBackend* m_appBackend;
        Application* m_app;
        int m_progress;
        QString m_comment;
};

#endif // TRANSACTIONLISTENER_H
