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

#ifndef AKABEIUPDATER_H
#define AKABEIUPDATER_H

#include <resources/AbstractBackendUpdater.h>
#include <akabeiclienttransactionhandler.h>

class AkabeiBackend;

namespace Akabei {
    class Package;
};

class AkabeiUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
public:
    AkabeiUpdater(AkabeiBackend * parent);
    ~AkabeiUpdater();
    
    virtual QList< QAction* > messageActions() const;
    virtual quint64 downloadSpeed() const;
    virtual QString statusDetail() const;
    virtual QString statusMessage() const;
    virtual bool isProgressing() const;
    virtual bool isCancelable() const;
    virtual bool isAllMarked() const;
    virtual bool isMarked(AbstractResource* res) const;
    virtual QDateTime lastUpdate() const;
    virtual QList< AbstractResource* > toUpdate() const;
    virtual void addResources(const QList< AbstractResource* >& apps);
    virtual void removeResources(const QList< AbstractResource* >& apps);
    virtual long unsigned int remainingTime() const;
    virtual qreal progress() const;
    virtual bool hasUpdates() const;
    virtual void prepare();
    
public Q_SLOTS:
    virtual void start();
    void transactionCreated(AkabeiClient::Transaction*);
    void validationFinished(bool);
    void finished(bool);
    void speedChanged(Akabei::Package*,AkabeiClient::DownloadInformation);
    void progressChanged(int);
    void packageStarted(Akabei::Package*);
    void phaseChanged(AkabeiClient::TransactionProgress::Phase);
    void newTransactionMessage(const QString &);
    
private:
    AkabeiBackend * m_backend;
    QList<AbstractResource*> m_marked;
    AkabeiClient::Transaction * m_transaction;
    QString m_statusDetail;
    QString m_statusMessage;
    bool m_isProgressing;
    quint64 m_downloadSpeed;
    qreal m_progress;
};

#endif // AKABEIUPDATER_H
