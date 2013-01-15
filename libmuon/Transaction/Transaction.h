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

#include "libmuonprivate_export.h"

class AbstractResource;

class MUONPRIVATE_EXPORT Transaction : public QObject
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
        InstallRole = 0,
        RemoveRole,
        ChangeAddonsRole
    };
    Q_ENUMS(Role)

    Transaction(QObject *parent, AbstractResource *resource,
                 Transaction::Role role);
    Transaction(QObject *parent, AbstractResource *resource,
                 Transaction::Role role, AddonList addons);

    AbstractResource *resource() const;
    Role role() const;
    Status status() const;
    AddonList addons() const;
    bool isCancellable() const;
    int progress() const;

    void setStatus(Status status);
    void setCancellable(bool isCancellable);
    void setProgress(int progress);
    void cancel();

private:
    AbstractResource *m_resource;
    Role m_role;
    Status m_status;
    AddonList m_addons;
    bool m_isCancellable;
    int m_progress;

signals:
    void statusChanged(Transaction::Status status);
    void cancellableChanged(bool cancellable);
    void progressChanged(int progress);
};

#endif // TRANSACTION_H
