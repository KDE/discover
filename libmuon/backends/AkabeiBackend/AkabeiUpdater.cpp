/*
 * <one line to give the library's name and an idea of what it does.>
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
#include "AkabeiUpdater.h"
#include "AkabeiBackend.h"
#include "AkabeiResource.h"
#include "AkabeiQuestion.h"
#include <resources/AbstractResource.h>
#include <QDateTime>
#include <akabeiclientbackend.h>
#include <akabeiclienttransactionhandler.h>
#include <KDebug>
#include <KMessageBox>
#include <KLocale>

AkabeiUpdater::AkabeiUpdater(AkabeiBackend * parent)
  : AbstractBackendUpdater(parent),
    m_backend(parent),
    m_transaction(0),
    m_isProgressing(0),
    m_downloadSpeed(0)
{

}

AkabeiUpdater::~AkabeiUpdater()
{

}

void AkabeiUpdater::prepare()
{
    kDebug();
    foreach (AbstractResource * res, m_backend->allResources()) {
        if (res->canUpgrade()) {
            m_marked.append(res);
        }
    }
}

void AkabeiUpdater::start()
{
    if (m_backend->isTransactionRunning()) {
        KMessageBox::error(0, i18n("Another transaction is still running!"), i18n("Error"));
        return;
    }
    kDebug();
    m_isProgressing = true;
    emit progressingChanged(m_isProgressing);
    AkabeiClient::Queue * queue = AkabeiClient::Backend::instance()->queue();
    queue->clear();
    foreach (AbstractResource * res, m_marked) {
        AkabeiResource * akabeiResource = qobject_cast<AkabeiResource*>(res);
        queue->addPackage(akabeiResource->package(), AkabeiClient::Update);
    }
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(transactionCreated(AkabeiClient::Transaction*)), SLOT(transactionCreated(AkabeiClient::Transaction*)));
    connect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), SIGNAL(downloadInformationChanged(Akabei::Package*,AkabeiClient::DownloadInformation)), SLOT(speedChanged(Akabei::Package*,AkabeiClient::DownloadInformation)));
    //connect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), SIGNAL(progressChanged(int)), SLOT(progressChanged(int)), Qt::QueuedConnection);
    connect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), SIGNAL(packageStarted(Akabei::Package*)), SLOT(packageStarted(Akabei::Package*)));
    connect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), SIGNAL(phaseChanged(AkabeiClient::TransactionProgress::Phase)), SLOT(phaseChanged(AkabeiClient::TransactionProgress::Phase)));
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(newTransactionMessage(QString)), SLOT(newTransactionMessage(QString)));

    AkabeiClient::Backend::instance()->transactionHandler()->start(Akabei::NoProcessingOption);
}

void AkabeiUpdater::phaseChanged(AkabeiClient::TransactionProgress::Phase )
{
    switch (AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress()->phase()) {
        case AkabeiClient::TransactionProgress::Downloading:
            m_statusMessage = i18n("Started downloading packages...");
            emit statusMessageChanged(m_statusMessage);
            return;
        case AkabeiClient::TransactionProgress::Processing:
            m_statusMessage = i18n("Started processing packages...");
            emit statusMessageChanged(m_statusMessage);
            return;
        default:
            break;
    };
}

void AkabeiUpdater::newTransactionMessage(const QString &message)
{
    m_statusMessage = message;
    emit statusMessageChanged(message);
}

void AkabeiUpdater::packageStarted(Akabei::Package* p)
{
    switch (AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress()->phase()) {
        case AkabeiClient::TransactionProgress::Downloading:
            m_statusDetail = i18n("Started downloading %1...", p->name());
            emit statusDetailChanged(m_statusDetail);
            return;
        case AkabeiClient::TransactionProgress::Processing:
            m_statusDetail = i18n("Started processing %1...", p->name());
            emit statusDetailChanged(m_statusDetail);
            return;
        default:
            break;
    };
}

void AkabeiUpdater::progressChanged(int progress)
{
    m_progress = progress;
    emit progressChanged(m_progress);
}

void AkabeiUpdater::speedChanged(Akabei::Package*, AkabeiClient::DownloadInformation )
{
    m_downloadSpeed = 0;
    foreach (AkabeiClient::DownloadInformation info, AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress()->downloadInformation().values()) {
        m_downloadSpeed += info.downloadSpeed();
    }
    emit downloadSpeedChanged(m_downloadSpeed);
}

void AkabeiUpdater::transactionCreated(AkabeiClient::Transaction* transaction)
{
    m_transaction = transaction;
    disconnect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(transactionCreated(AkabeiClient::Transaction*)), this, SLOT(transactionCreated(AkabeiClient::Transaction*)));
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(validationFinished(bool)), SLOT(validationFinished(bool)));
    
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

void AkabeiUpdater::validationFinished(bool success)
{
    kDebug();
    disconnect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(validationFinished(bool)), this, SLOT(validationFinished(bool)));
    connect(AkabeiClient::Backend::instance()->transactionHandler(), SIGNAL(finished(bool)), SLOT(finished(bool)));
    
    if (!success) {
        finished(false);
        return;
    }
    
    AkabeiClient::Backend::instance()->transactionHandler()->process();
}

void AkabeiUpdater::finished(bool success)
{
    kDebug();
    m_isProgressing = false;
    emit progressingChanged(m_isProgressing);

    kDebug() << "Finished" << success;
    if (!success) {
        QString err;
        foreach (const Akabei::Error &error, m_transaction->errors()) {
            err.append(" " + error.description());
        }
        if (err.isEmpty())
            err = i18n("Something went wrong!");
        KMessageBox::error(0, err, i18n("Error"));
    }
    disconnect(AkabeiClient::Backend::instance()->transactionHandler(), 0, this, 0);
    disconnect(AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress(), 0, this, 0);
    m_transaction = 0;
    m_backend->reload();
}

QList< AbstractResource* > AkabeiUpdater::toUpdate() const
{
    return m_marked;
}

void AkabeiUpdater::addResources(const QList< AbstractResource* >& apps)
{
    m_marked << apps;
}

void AkabeiUpdater::removeResources(const QList< AbstractResource* >& apps)
{
    foreach (AbstractResource * res, apps) {
        m_marked.removeAll(res);
    }
}

QList< QAction* > AkabeiUpdater::messageActions() const
{
    return QList<QAction*>();
}

quint64 AkabeiUpdater::downloadSpeed() const
{
    return m_downloadSpeed;
}

QString AkabeiUpdater::statusDetail() const
{
    return m_statusDetail;
}

QString AkabeiUpdater::statusMessage() const
{
    return m_statusMessage;
}

bool AkabeiUpdater::isProgressing() const
{
    return m_isProgressing;
}

bool AkabeiUpdater::isCancelable() const
{
    return false;
}

bool AkabeiUpdater::isAllMarked() const
{
    return m_backend->updatesCount() == m_marked.size();
}

bool AkabeiUpdater::isMarked(AbstractResource* res) const
{
    return m_marked.contains(res);
}

QDateTime AkabeiUpdater::lastUpdate() const
{
    return QDateTime::currentDateTime();//FIXME
}

bool AkabeiUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

qreal AkabeiUpdater::progress() const
{
    return m_progress;
}

long unsigned int AkabeiUpdater::remainingTime() const
{
    return AkabeiClient::Backend::instance()->transactionHandler()->transactionProgress()->estimatedTime().secsTo(QTime(0, 0, 0));
}
