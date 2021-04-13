#ifndef OSTREERPMTRANSACTION_H
#define OSTREERPMTRANSACTION_H

#include "rpmostree1.h"
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
    // void proceed() override;

private Q_SLOTS:
    void finishTransaction(/*bool, QString*/);
    void displayTransactionMessage(QString);
    void taskBegin(QString);
    void taskEnd(QString);
    void iterateTransaction(QString, unsigned int);

private:
    RpmOstreeResource *m_app;
    OrgProjectatomicRpmostree1TransactionInterface *m_transaction;
    QString transactionUpdatePath;
    bool m_isDeploymentUpdate;
    void setupDeploymentTransaction();
    void startTransaction(QDBusPendingCallWatcher *watcher);
};

#endif