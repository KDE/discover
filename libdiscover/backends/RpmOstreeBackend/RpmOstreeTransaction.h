/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
 
#ifndef OSTREERPMTRANSACTION_H
#define OSTREERPMTRANSACTION_H

#include "RpmOstreeDBusInterface.h"

#include <QDBusPendingCallWatcher>
#include <Transaction/Transaction.h>

class RpmOstreeResource;
class RpmOstreeTransaction : public Transaction
{
    Q_OBJECT
public:
    RpmOstreeTransaction(RpmOstreeResource *app, Role role, QString path, bool isDeploymentUpdate);
    RpmOstreeTransaction(RpmOstreeResource *app, const AddonList &list, Role role, QString path, bool isDeploymentUpdate);

    void cancel() override;

private Q_SLOTS:
    void finishTransaction(/*bool, QString*/);
    void displayTransactionMessage(QString);
    void taskBegin(QString);
    void taskEnd(QString);
    void iterateTransaction(QString, unsigned int);

private:
    RpmOstreeResource *m_app;
    QString transactionUpdatePath;
    bool m_isDeploymentUpdate;
    void setupDeploymentTransaction();
    void startTransaction(QDBusPendingCallWatcher *watcher);
};

#endif