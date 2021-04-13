#include "RpmOstreeTransaction.h"
#include "RpmOstreeBackend.h"
#include "RpmOstreeResource.h"
#include <KRandom>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFile>
#include <QTimer>

RpmOstreeTransaction::RpmOstreeTransaction(RpmOstreeResource *app, Role role, QString path, bool isDeploymentUpdate)
    : RpmOstreeTransaction(app, {}, role, path, isDeploymentUpdate)
{
}

RpmOstreeTransaction::RpmOstreeTransaction(RpmOstreeResource *app, const AddonList &addons, Transaction::Role role, QString path, bool isDeploymentUpdate)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
    , transactionUpdatePath(path)
    , m_isDeploymentUpdate(isDeploymentUpdate)
{
    setCancellable(true);
    setStatus(DownloadingStatus);
    setupDeploymentTransaction();
}

void RpmOstreeTransaction::iterateTransaction(QString text, unsigned int percentage)
{
    if (progress() < 100) {
        setProgress(percentage);
    }
}

void RpmOstreeTransaction::cancel()
{
    QDBusPendingReply<> reply = m_transaction->Cancel();
    reply.waitForFinished();
    setStatus(CancelledStatus);
}

void RpmOstreeTransaction::setupDeploymentTransaction()
{
    QDBusConnection socketConnection = QDBusConnection::connectToPeer(transactionUpdatePath, QStringLiteral("org.projectatomic.rpmostree1"));
    m_transaction =
        new OrgProjectatomicRpmostree1TransactionInterface(QStringLiteral("org.projectatomic.rpmostree1"), QStringLiteral("/"), socketConnection, this);

    connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::ProgressEnd, this, &RpmOstreeTransaction::finishTransaction);
    QDBusPendingReply<bool> transactionReply = m_transaction->Start();

    /*bool connected1 = connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::TaskBegin , this, &RpmOstreeTransaction::taskBegin);
    bool connected2 = connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::Message, this, &RpmOstreeTransaction::displayTransactionMessage);
    bool connected3 = connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::PercentProgress, this, &RpmOstreeTransaction::iterateTransaction);
    bool connected4 = connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::TaskEnd, this, &RpmOstreeTransaction::taskEnd);
    bool connected5 = connect(m_transaction, &OrgProjectatomicRpmostree1TransactionInterface::Finished, this, &RpmOstreeTransaction::finishTransaction);

    qWarning() << "taskBegin: " << connected1 << Qt::endl;
    qWarning() << "displayTransactionMessage: " << connected2 << Qt::endl;
    qWarning() << "iterateTransaction: " << connected3 << Qt::endl;
    qWarning() << "taskEnd: " << connected4 << Qt::endl;
    qWarning() << "finishTransaction: " << connected5 << Qt::endl; */
    /*QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(transactionReply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this] {
        setStatus(DoneStatus);
    }); */

    // connect(watcher, &QDBusPendingCallWatcher::finished, this, &RpmOstreeTransaction::startTransaction);
}

void RpmOstreeTransaction::displayTransactionMessage(QString message)
{
    qWarning() << message << Qt::endl;
}

void RpmOstreeTransaction::taskBegin(QString message)
{
    qWarning() << message << Qt::endl;
}

void RpmOstreeTransaction::taskEnd(QString message)
{
    qWarning() << message << Qt::endl;
}

void RpmOstreeTransaction::finishTransaction(/*bool sucess, QString error_message*/)
{
    AbstractResource::State newState;
    switch (role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    setStatus(DoneStatus);
    QFile file(QStringLiteral("/tmp/discover-ostree-changed"));
    file.open(QFile::WriteOnly);
    file.write("I love bananas");
    file.close();
    /*if(sucess) {
       AbstractResource::State newState;
       switch(role()) {
       case InstallRole:
       case ChangeAddonsRole:
          newState = AbstractResource::Installed;
          break;
       case RemoveRole:
          newState = AbstractResource::None;
          break;
       }
       m_app->setState(newState);
       setStatus(DoneStatus);
       QFile file(QStringLiteral("/tmp/discover-ostree-changed"));
       file.open(QFile::WriteOnly);
       file.write("I love bananas");
       file.close();
       deleteLater();
    }
    else {
        qWarning() << error_message << Qt::endl;
        setStatus(DoneWithErrorStatus);
    } */
}
