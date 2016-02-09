/*
 * Copyright 2013  Lukas Appelhans <l.appelhans@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AKABEITRANSACTION_H
#define AKABEITRANSACTION_H

#include <Transaction/Transaction.h>
#include <akabeiclienttransactionhandler.h>

class AkabeiBackend;
class AkabeiTransaction : public Transaction
{
    Q_OBJECT
public:
    AkabeiTransaction(AkabeiBackend* parent, AbstractResource* resource, Transaction::Role role);
    AkabeiTransaction(AkabeiBackend* parent, AbstractResource* resource, Transaction::Role role, const AddonList& addons);
    ~AkabeiTransaction();
    
public Q_SLOTS:
    void transactionCreated(AkabeiClient::Transaction * transaction);
    void validationFinished(bool);
    void finished(bool);
    void phaseChanged(AkabeiClient::TransactionProgress::Phase);
    void start();
    void transactionMessage(const QString &message);
    
private:
    QStringList m_transactionMessages;
    AkabeiBackend * m_backend;
    AkabeiClient::Transaction * m_transaction;
};

#endif // AKABEITRANSACTION_H
