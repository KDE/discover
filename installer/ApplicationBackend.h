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

#ifndef APPLICATIONBACKEND_H
#define APPLICATIONBACKEND_H

#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QSet>

#include <LibQApt/Globals>
#include <LibQApt/Package>

namespace QApt {
    class Backend;
}

namespace DebconfKde
{
    class DebconfGui;
}

class Application;
class ReviewsBackend;
class Transaction;

class ApplicationBackend : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationBackend(QObject *parent=0);
    ~ApplicationBackend();

    Q_SCRIPTABLE ReviewsBackend *reviewsBackend() const;
    QList<Application *> applicationList() const;
    QSet<QString> appOrigins() const;
    QSet<QString> installedAppOrigins() const;
    QPair<QApt::WorkerEvent, Transaction *> workerState() const;
    QList<Transaction *> transactions() const;
    QStringList launchList() const;

    bool confirmRemoval(Transaction *transaction);
    void markTransaction(Transaction *transaction);
    void addTransaction(Transaction *transaction);
    void clearLaunchList();
    
public slots:
    //helper functions
    void installApplication(Application *app, const QHash<QApt::Package *, QApt::Package::State> &addons);
    void installApplication(Application *app);
    void removeApplication(Application *app);
    void cancelTransaction(Application *app);

private:
    QApt::Backend *m_backend;
    ReviewsBackend *m_reviewsBackend;

    QList<Application *> m_appList;
    QSet<QString> m_originList;
    QSet<QString> m_instOriginList;
    QStringList m_appLaunchList;
    QStringList m_pkgBlacklist;
    QQueue<Transaction *> m_queue;
    Transaction *m_currentTransaction;
    QPair<QApt::WorkerEvent, Transaction *> m_workerState;

    DebconfKde::DebconfGui *m_debconfGui;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void reload();

private Q_SLOTS:
    void init();
    void runNextTransaction();
    void workerEvent(QApt::WorkerEvent event);
    void errorOccurred(QApt::ErrorCode error, const QVariantMap &details);
    void updateDownloadProgress(int percentage);
    void updateCommitProgress(const QString &text, int percentage);

Q_SIGNALS:
    void appBackendReady();
    void reloadStarted();
    void reloadFinished();
    void workerEvent(QApt::WorkerEvent event, Transaction *app);
    void errorSignal(QApt::ErrorCode code, const QVariantMap &details);
    void progress(Transaction *transaction, int progress);
    void transactionCancelled(Application *app);
    void xapianReloaded();
};

#endif
