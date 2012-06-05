/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "libmuonprivate_export.h"
#include <resources/AbstractResourcesBackend.h>

class Transaction;
class AbstractResource;
class AbstractResourcesBackend;

class MUONPRIVATE_EXPORT TransactionListener : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString comment READ comment NOTIFY commentChanged)
    Q_PROPERTY(AbstractResource* resource READ resource WRITE setResource NOTIFY resourceChanged)
    Q_PROPERTY(AbstractResourcesBackend* backend READ backend WRITE setBackend) //TODO: port to ResourcesModel
    Q_PROPERTY(bool isActive READ isActive NOTIFY running)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY downloading)
    public:
        explicit TransactionListener(QObject* parent = 0);
        virtual ~TransactionListener();
        void setBackend(AbstractResourcesBackend* appBackend);
        void setResource(AbstractResource* app);
        int progress() const;
        QString comment() const;
        AbstractResource* resource() const;
        AbstractResourcesBackend* backend() const;
        void init();
        bool isActive() const;
        bool isDownloading() const;

    signals:
        void progressChanged();
        void commentChanged();
        void resourceChanged();
        void running(bool isRunning);
        void downloading(bool isDownloading);
        void cancelled();

    private slots:
        void updateProgress(Transaction*,int);
        void transactionCancelled(Transaction*);
        void transactionRemoved(Transaction*);
        void workerEvent(TransactionStateTransition, Transaction*);

    private:
        void setDownloading(bool);
        void showTransactionState(Transaction* transaction);
        void setStateComment(Transaction* transaction);
        
        AbstractResourcesBackend* m_appBackend;
        AbstractResource* m_app;
        int m_progress;
        QString m_comment;
        bool m_downloading;
};

#endif // TRANSACTIONLISTENER_H
