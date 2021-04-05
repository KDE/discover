/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKTRANSACTIONTHREAD_H
#define FLATPAKTRANSACTIONTHREAD_H

#include <QThread>
#include <Transaction/Transaction.h>

extern "C" {
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

class FlatpakResource;
class FlatpakTransactionThread : public QThread
{
    Q_OBJECT
public:
    FlatpakTransactionThread(FlatpakResource *app, Transaction::Role role);
    ~FlatpakTransactionThread() override;

    void cancel();
    void run() override;

    int progress() const
    {
        return m_progress;
    }
    void setProgress(int progress);
    void setSpeed(quint64 speed);

    QString errorMessage() const;
    bool result() const;

    void addErrorMessage(const QString &error);

Q_SIGNALS:
    void progressChanged(int progress);
    void speedChanged(quint64 speed);
    void passiveMessage(const QString &msg);

private:
    FlatpakTransaction *m_transaction;

    bool m_result = false;
    int m_progress = 0;
    quint64 m_speed = 0;
    QString m_errorMessage;
    GCancellable *m_cancellable;
    FlatpakResource *const m_app;
    const Transaction::Role m_role;
};

#endif // FLATPAKTRANSACTIONJOB_H
