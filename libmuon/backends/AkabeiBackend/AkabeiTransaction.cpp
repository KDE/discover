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

#include "AkabeiTransaction.h"
#include "AkabeiResource.h"
#include "AkabeiBackend.h"
#include "AkabeiQuestion.h"
#include <Transaction/TransactionModel.h>
#include <akabeiclientqueue.h>
#include <akabeiclientbackend.h>
#include <akabeiclienttransactionhandler.h>
#include <KDebug>
#include <KMessageBox>
#include <QButtonGroup>
#include <KLocale>
#include <KDialog>
#include <akabeidatabase.h>
#include <akabeiquery.h>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
//FIXME: Transaction messages? How to show them properly?

AkabeiTransaction::AkabeiTransaction(AkabeiBackend* parent, AbstractResource* resource, Transaction::Role role)
  : Transaction(parent, resource, role),
    m_backend(parent),
    m_transaction(0)
{
    setCancellable(false);
    setStatus(Transaction::QueuedStatus);
}

AkabeiTransaction::AkabeiTransaction(AkabeiBackend* parent, AbstractResource* resource, Transaction::Role role, AddonList addons)
  : Transaction(parent, resource, role, addons),
    m_backend(parent),
    m_transaction(0)
{
    setCancellable(false);
    setStatus(Transaction::QueuedStatus);
}

AkabeiTransaction::~AkabeiTransaction()
{

}

void AkabeiTransaction::start()
{
    AkabeiClient::Backend::instance()->queue()->clear();
    switch (role()) {
        case Transaction::InstallRole: {
            AkabeiResource * res = qobject_cast<AkabeiResource*>(resource());
            if (res->isInstalled()) {
                finished(true);
                return;
            }
            AkabeiClient::Backend::instance()->queue()->addPackage(res->package(), AkabeiClient::Install);
            break;
        }
        case Transaction::RemoveRole: {
            AkabeiResource * res = qobject_cast<AkabeiResource*>(resource());
            if (!res->isInstalled()) {
                finished(true);
                return;
            }
            AkabeiClient::Backend::instance()->queue()->addPackage(res->installedPackage(), AkabeiClient::Remove);
            break;
        }
        case Transaction::ChangeAddonsRole:
            break;
    }
    foreach (const QString &toRemove, addons().addonsToRemove()) {
        AbstractResource * res = m_backend->resourceByPackageName(toRemove);
        if (res) {
            AkabeiClient::Backend::instance()->queue()->addPackage(qobject_cast<AkabeiResource*>(res)->installedPackage(), AkabeiClient::Remove);
        } else {
            Akabei::Package::List pkgs = Akabei::Backend::instance()->localDatabase()->queryPackages("SELECT * FROM packages JOIN provides WHERE provides.provides LIKE \"" + toRemove + "\"");
            foreach (Akabei::Package * p, pkgs)
                AkabeiClient::Backend::instance()->queue()->addPackage(p, AkabeiClient::Remove);
        }
    }
    foreach (const QString &toInstall, addons().addonsToInstall()) {
        AbstractResource * res = m_backend->resourceByPackageName(toInstall);
        if (res) {
            AkabeiClient::Backend::instance()->queue()->addPackage(qobject_cast<AkabeiResource*>(res)->package(), AkabeiClient::Install);
        } //FIXME: Handle providers in the else clause?
    }
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(transactionCreated(AkabeiClient::Transaction*)), SLOT(transactionCreated(AkabeiClient::Transaction*)));
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(validationFinished(bool)), SLOT(validationFinished(bool)));
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(finished(bool)), SLOT(finished(bool)));
    connect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), SIGNAL(phaseChanged(AkabeiClient::TransactionProgress::Phase)), SLOT(phaseChanged(AkabeiClient::TransactionProgress::Phase)));

    foreach (AkabeiClient::QueueItem * item, AkabeiClient::Backend::instance()->queue()->items())
        kDebug() << "QUEUE ITEM" << item->package()->name() << (item->action() == AkabeiClient::Install);
    
    AkabeiClient::Backend::instance()->transactionHandler()->start(Akabei::ProcessingOption::NoProcessingOption);
}

void AkabeiTransaction::phaseChanged(AkabeiClient::TransactionProgress::Phase phase)
{
    switch (phase) {
        case AkabeiClient::TransactionProgress::Downloading:
            setStatus(Transaction::DownloadingStatus);
            break;
        case AkabeiClient::TransactionProgress::Processing:
            setStatus(Transaction::CommittingStatus);
            break;
        default:
            break;
    };
}

void AkabeiTransaction::transactionCreated(AkabeiClient::Transaction* transaction)
{
    kDebug() << "Transaction created";
    m_transaction = transaction;
    foreach (AkabeiClient::TransactionQuestion * q, transaction->questions()) {
        AkabeiQuestion question(q);
        q->setAnswer(question.ask());
    }
    if (!transaction->isValid()) {
        finished(false);
        return;
    } else {
        kDebug() << "Continue with transaction";
        AkabeiClient::Backend::instance()->transactionHandler()->validate();
    }
}

void AkabeiTransaction::validationFinished(bool successful)
{
    if (!successful) {
        finished(false);
        return;
    }
    
    AkabeiClient::Backend::instance()->transactionHandler()->process();
}

void AkabeiTransaction::finished(bool successful)
{
    kDebug() << "Finished" << successful;
    if (!successful) {
        QString err;
        foreach (const Akabei::Error &error, m_transaction->errors()) {
            err.append(" " + error.description());
        }
        if (err.isEmpty())
            err = i18n("Something went wrong!");
        KMessageBox::error(0, err, i18n("Error"));
    }
    setStatus(Transaction::DoneStatus);
    disconnect(AkabeiClient::Backend::instance()->transactionHandler(), 0, this, 0);
    disconnect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), 0, this, 0);
    m_backend->removeFromQueue(this);
    deleteLater();
}
