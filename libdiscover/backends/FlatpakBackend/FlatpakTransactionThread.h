/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "flatpak-helper.h"
#include <gio/gio.h>
#include <glib.h>

#include <QHash>
#include <QMap>
#include <QMutex>
#include <QRunnable>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>
#include <Transaction/Transaction.h>

#include <QThreadPool>
class FlatpakThreadPool : public QThreadPool
{
public:
    [[nodiscard]] static FlatpakThreadPool *instance()
    {
        static FlatpakThreadPool pool;
        return &pool;
    }

private:
    FlatpakThreadPool()
    {
        // We only run a single transaction at a time but do so in a thread pool of size 1.
        // We can't do more threading because we'd exhaust resources.
        // https://bugs.kde.org/show_bug.cgi?id=474231
        // But also because suse enforces authentication which means N auth dialogs for N threads.
        // https://bugs.kde.org/show_bug.cgi?id=466559
        setMaxThreadCount(1);
    }
};

class FlatpakJobTransaction;
class FlatpakResource;
class FlatpakTransactionThread : public QObject, public QRunnable
{
    Q_OBJECT
public:
    /** Mapping of repositories where a key is an installation path and a value is a list of names */
    using Repositories = QMap<QString, QStringList>;

    FlatpakTransactionThread(Transaction::Role role, FlatpakInstallation *installation);
    ~FlatpakTransactionThread() override;

    void addJobTransaction(FlatpakJobTransaction *jobTransaction);
    void setCurrentRef(const char *ref);

    void cancel();
    void run() override;

    void setProgress(int progress);
    void setSpeed(quint64 speed);

    void addErrorMessage(const QString &error);
    void operationError(GError *error);
    bool end_of_lifed_with_rebase(const char *remote, const char *ref, const char *reason, const char *rebased_to_ref, const char **previous_ids);
    int choose_remote_for_ref(const char *for_ref, const char *runtime_ref, const char *const *remotes, unsigned int remotesCount);
    void proceed();

Q_SIGNALS: // Signals vastly simplify our live with regards to threading since Qt schedules them into the correct thread
    void progressChanged(int progress);
    void speedChanged(quint64 speed);
    void passiveMessage(const QString &msg);
    void webflowStarted(const QUrl &url, int id);
    void webflowDone(int id);
    void finished(bool cancelled, const QString &errorMessage, const FlatpakTransactionThread::Repositories &addedRepositories, bool success);
    void proceedRequest(const QString &title, const QString &description);
    void statusChanged(Transaction::Status status);

private:
    static gboolean
    add_new_remote_cb(FlatpakTransaction * /*object*/, gint /*reason*/, gchar *from_id, gchar *suggested_remote_name, gchar *url, gpointer user_data);
    static void progress_changed_cb(FlatpakTransactionProgress *progress, gpointer user_data);
    static void
    new_operation_cb(FlatpakTransaction * /*object*/, FlatpakTransactionOperation *operation, FlatpakTransactionProgress *progress, gpointer user_data);
    void fail(const char *refName, GError *error);

    QString errorMessage() const;
    bool cancelled() const;
    int progress() const;
    Repositories addedRepositories() const;
    [[nodiscard]] bool success() const;
    void finishAllJobTransactions();
    [[nodiscard]] std::optional<QString> refToAppRef(const QString &ref) const;

    static gboolean webflowStart(FlatpakTransaction *transaction, const char *remote, const char *url, GVariant *options, guint id, gpointer user_data);
    static void webflowDoneCallback(FlatpakTransaction *transaction, GVariant *options, guint id, gpointer user_data);
    [[nodiscard]] bool setupTransaction();

    GCancellable *m_cancellable;
    FlatpakTransaction *m_transaction = nullptr;
    int m_progress = 0;
    quint64 m_speed = 0;
    QString m_errorMessage;
    const Transaction::Role m_role;
    FlatpakInstallation *m_installation;
    QMap<QString, QStringList> m_addedRepositories;
    QMutex m_proceedMutex;
    QWaitCondition m_proceedCondition;
    bool m_proceed = false;
    // WARNING: Beware that the JobTransaction objects live in another thread! Do not call them directly. Use Signals.
    QHash<QString, QPointer<FlatpakJobTransaction>> m_jobTransactionsByRef;
    void *m_currentJobTransaction = nullptr; // void so you can't accidentally call into it. Use Signals. This is a different thread!
    QString m_initializationErrorMessage;
    std::optional<bool> m_operationSuccess;
    // Tries to runtimes to app refs when information is available. Notably, calls to choose_remote_for_ref will produce a mapping here.
    // The intent is to allow tieing runtimes (which we consider internal) to application refs, so we can display them in the UI.
    QHash<QString, QString> m_runtimeToAppRef;

    QVector<int> m_webflows;
};
