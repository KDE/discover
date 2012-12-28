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

#include <QtCore/QObject>

#include "Transaction.h"
#include "libmuonprivate_export.h"

class AbstractResource;

class MUONPRIVATE_EXPORT TransactionListener : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource* resource READ resource WRITE setResource NOTIFY resourceChanged)
    Q_PROPERTY(bool isCancellable READ isCancellable NOTIFY cancellableChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY running)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
public:
    explicit TransactionListener(QObject *parent = nullptr);
    
    AbstractResource *resource() const;
    bool isCancellable() const;
    bool isActive() const;
    QString statusText() const;

    void setResource(AbstractResource* resource);

private:
    AbstractResource *m_resource;
    Transaction *m_transaction;

private slots:
    void transactionAdded(Transaction *trans);
    void transactionStatusChanged(Transaction::Status status);

signals:
    void resourceChanged();
    void cancellableChanged();
    void running();
    void statusTextChanged();
};

#endif // TRANSACTIONLISTENER_H
