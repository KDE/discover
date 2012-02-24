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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QtCore/QHash>

#include <LibQApt/Package>

#include "libmuonprivate_export.h"

class Application;

enum TransactionState {
    InvalidState = 0,
    QueuedState = 1,
    RunningState = 2,
    DoneState = 3
};

enum TransactionAction {
    InvalidAction = 0,
    InstallApp = 1,
    RemoveApp = 2,
    ChangeAddons = 3
};

class MUONPRIVATE_EXPORT Transaction
{
public:
    explicit Transaction (Application *app, TransactionAction);
    explicit Transaction (Application *app, TransactionAction,
                          const QHash<QApt::Package *, QApt::Package::State> &addons);
    ~Transaction();

    void setState(TransactionState state);

    Application *application() const;
    TransactionAction action() const;
    TransactionState state() const;
    QHash<QApt::Package *, QApt::Package::State> addons() const;

private:
    Application *m_application;
    TransactionAction m_action;
    TransactionState m_state;
    QHash<QApt::Package *, QApt::Package::State> m_addons;
};

#endif
