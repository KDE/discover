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

#ifndef TRANSACTION_H
#define TRANSACTION_H

// Qt includes
#include <QtCore/QObject>

// Own includes
#include "AddonList.h"

#include "discovercommon_export.h"

class AbstractResource;

/**
 * \class Transaction  Transaction.h "Transaction.h"
 *
 * \brief This is the base class of all transactions.
 * 
 * When there are transactions running inside Muon, the backends should
 * provide the corresponding Transaction objects with proper information.
 */
class DISCOVERCOMMON_EXPORT Transaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(AbstractResource* resource READ resource CONSTANT)
    Q_PROPERTY(Role role READ role CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool isCancellable READ isCancellable NOTIFY cancellableChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    enum Status {
        /// Not queued, newly created
        SetupStatus = 0,
        /// Queued, but not yet run
        QueuedStatus,
        /// Transaction is in the downloading phase
        DownloadingStatus,
        /// Transaction is doing an installation/removal
        CommittingStatus,
        /// Transaction is done
        DoneStatus
    };
    Q_ENUMS(Status)

    enum Role {
        ///The transaction is going to install a resource
        InstallRole = 0,
        ///The transaction is going to remove a resource
        RemoveRole,
        ///The transaction is going to change the addons of a resource
        ChangeAddonsRole
    };
    Q_ENUMS(Role)

    Transaction(QObject *parent, AbstractResource *resource,
                 Transaction::Role role, const AddonList &addons = {});

    /**
     * @returns the AbstractResource which this transaction works with
     */
    AbstractResource *resource() const;
    /**
     * @returns the role which this transaction executes
     */
    Role role() const;
    /**
     * @returns the current status
     */
    Status status() const;
    /**
     * @returns the addons which this transaction works on
     */
    AddonList addons() const;
    /**
     * @returns true when the transaction can be canceled
     */
    bool isCancellable() const;
    /**
     * @returns a percentage of how much the transaction is already done
     */
    int progress() const;

    /**
     * Sets the status of the transaction
     * @param status the new status
     */
    void setStatus(Status status);
    /**
     * Sets whether the transaction can be canceled or not
     * @param isCancellable should be true if the transaction can be canceled
     */
    void setCancellable(bool isCancellable);
    /**
     * Sets the progress of the transaction
     * @param progress this should be a percentage of how much of the transaction is already done
     */
    void setProgress(int progress);
    /**
     * Cancels the transaction
     */
    void cancel();

private:
    AbstractResource * const m_resource;
    const Role m_role;
    Status m_status;
    const AddonList m_addons;
    bool m_isCancellable;
    int m_progress;

Q_SIGNALS:
    /**
     * This gets emitted when the status of the transaction changed
     */
    void statusChanged(Transaction::Status status);
    /**
     * This gets emitted when the ability to cancel the transaction or not changed
     */
    void cancellableChanged(bool cancellable);
    /**
     * This gets emitted when the transaction changed the percentage of how much of it is already done
     */
    void progressChanged(int progress);
};

#endif // TRANSACTION_H
