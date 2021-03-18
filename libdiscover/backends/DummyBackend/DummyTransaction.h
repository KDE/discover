/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DUMMYTRANSACTION_H
#define DUMMYTRANSACTION_H

#include <Transaction/Transaction.h>

class DummyResource;
class DummyTransaction : public Transaction
{
    Q_OBJECT
public:
    DummyTransaction(DummyResource *app, Role role);
    DummyTransaction(DummyResource *app, const AddonList &list, Role role);

    void cancel() override;
    void proceed() override;

private Q_SLOTS:
    void iterateTransaction();
    void finishTransaction();

private:
    bool m_iterate = true;
    DummyResource *m_app;
};

#endif // DUMMYTRANSACTION_H
