/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakTransactionThread.h"
#include "FlatpakResource.h"

#include <KLocalizedString>
#include <QDebug>
#include <QDesktopServices>
#include <QScopeGuard>

static int FLATPAK_CLI_UPDATE_FREQUENCY = 150;

gboolean FlatpakTransactionThread::add_new_remote_cb(FlatpakTransaction *object,
                                                     gint /*reason*/,
                                                     gchar *from_id,
                                                     gchar *suggested_remote_name,
                                                     gchar *url,
                                                     gpointer user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;

    // TODO ask instead
    auto name = QString::fromUtf8(suggested_remote_name);
    obj->m_addedRepositories[FlatpakResource::installationPath(flatpak_transaction_get_installation(object))].append(name);
    Q_EMIT obj->passiveMessage(i18n("Adding remote '%1' in %2 from %3", name, QString::fromUtf8(url), QString::fromUtf8(from_id)));
    return true;
}

void FlatpakTransactionThread::progress_changed_cb(FlatpakTransactionProgress *progress, gpointer user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;

    g_autolist(GObject) ops = flatpak_transaction_get_operations(obj->m_transaction);
    g_autoptr(FlatpakTransactionOperation) op = flatpak_transaction_get_current_operation(obj->m_transaction);
    const int idx = g_list_index(ops, op);
    obj->setProgress(qMin<int>(99, (100 * idx + flatpak_transaction_progress_get_progress(progress)) / g_list_length(ops)));

#ifdef FLATPAK_VERBOSE_PROGRESS
    guint64 start_time = flatpak_transaction_progress_get_start_time(progress);
    guint64 elapsed_time = (g_get_monotonic_time() - start_time) / G_USEC_PER_SEC;
    if (elapsed_time > 0) {
        guint64 transferred = flatpak_transaction_progress_get_bytes_transferred(progress);
        obj->setSpeed(transferred / elapsed_time);
    }
#endif
}

void FlatpakTransactionThread::new_operation_cb(FlatpakTransaction * /*object*/,
                                                FlatpakTransactionOperation * /*operation*/,
                                                FlatpakTransactionProgress *progress,
                                                gpointer user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;

    g_signal_connect(progress, "changed", G_CALLBACK(&FlatpakTransactionThread::progress_changed_cb), obj);
    flatpak_transaction_progress_set_update_frequency(progress, FLATPAK_CLI_UPDATE_FREQUENCY);
}

void operation_error_cb(FlatpakTransaction * /*object*/, FlatpakTransactionOperation * /*operation*/, GError *error, gint /*details*/, gpointer user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;
    if (error) {
        obj->addErrorMessage(QString::fromUtf8(error->message));
    }
}

gboolean
FlatpakTransactionThread::webflowStart(FlatpakTransaction *transaction, const char *remote, const char *url, GVariant *options, guint id, gpointer user_data)
{
    Q_UNUSED(transaction);
    Q_UNUSED(options);

    QUrl webflowUrl(QString::fromUtf8(url));
    qDebug() << "starting web flow" << webflowUrl << remote << id;
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;
    obj->m_webflows << id;
    Q_EMIT obj->webflowStarted(webflowUrl, id);
    return true;
}

void FlatpakTransactionThread::webflowDoneCallback(FlatpakTransaction *transaction, GVariant *options, guint id, gpointer user_data)
{
    Q_UNUSED(transaction);
    Q_UNUSED(options);
    FlatpakTransactionThread *obj = (FlatpakTransactionThread *)user_data;
    obj->m_webflows << id;
    Q_EMIT obj->webflowDone(id);
    qDebug() << "webflow done" << id;
}

FlatpakTransactionThread::FlatpakTransactionThread(FlatpakResource *app, Transaction::Role role)
    : m_result(false)
    , m_app(app)
    , m_role(role)
{
    m_cancellable = g_cancellable_new();

    g_autoptr(GError) localError = nullptr;
    m_transaction = flatpak_transaction_new_for_installation(app->installation(), m_cancellable, &localError);
    if (localError) {
        addErrorMessage(QString::fromUtf8(localError->message));
        qWarning() << "Failed to create transaction" << m_errorMessage;
    } else {
        g_signal_connect(m_transaction, "add-new-remote", G_CALLBACK(add_new_remote_cb), this);
        g_signal_connect(m_transaction, "new-operation", G_CALLBACK(new_operation_cb), this);
        g_signal_connect(m_transaction, "operation-error", G_CALLBACK(operation_error_cb), this);

        if (qEnvironmentVariableIntValue("DISCOVER_FLATPAK_WEBFLOW")) {
            g_signal_connect(m_transaction, "webflow-start", G_CALLBACK(webflowStart), this);
            g_signal_connect(m_transaction, "webflow-done", G_CALLBACK(webflowDoneCallback), this);
        }
    }
}

FlatpakTransactionThread::~FlatpakTransactionThread()
{
    g_object_unref(m_transaction);
    g_object_unref(m_cancellable);
}

void FlatpakTransactionThread::cancel()
{
    for (int id : std::as_const(m_webflows))
        flatpak_transaction_abort_webflow(m_transaction, id);
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionThread::run()
{
    auto finish = qScopeGuard([this] {
        Q_EMIT finished();
    });

    if (!m_transaction)
        return;
    g_autoptr(GError) localError = nullptr;

    const QString refName = m_app->ref();

    if (m_role == Transaction::Role::InstallRole) {
        bool correct = false;
        if (m_app->state() == AbstractResource::Upgradeable && m_app->isInstalled()) {
            correct = flatpak_transaction_add_update(m_transaction, refName.toUtf8().constData(), nullptr, nullptr, &localError);
        } else if (m_app->flatpakFileType() == FlatpakResource::FileFlatpak) {
            g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
            if (!file) {
                qWarning() << "Failed to install bundled application" << refName;
                m_result = false;
                return;
            }
            correct = flatpak_transaction_add_install_bundle(m_transaction, file, nullptr, &localError);
        } else if (m_app->flatpakFileType() == FlatpakResource::FileFlatpakRef && m_app->resourceFile().isLocalFile()) {
            g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
            if (!file) {
                qWarning() << "Failed to install flatpakref application" << refName;
                m_result = false;
                return;
            }
            g_autoptr(GBytes) bytes = g_file_load_bytes(file, m_cancellable, nullptr, &localError);
            correct = flatpak_transaction_add_install_flatpakref(m_transaction, bytes, &localError);
        } else {
            correct = flatpak_transaction_add_install(m_transaction, //
                                                      m_app->origin().toUtf8().constData(),
                                                      refName.toUtf8().constData(),
                                                      nullptr,
                                                      &localError);
        }

        if (!correct) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            setProgress(100);
            qWarning() << "Failed to install" << m_app->flatpakFileType() << refName << ':' << m_errorMessage;
            return;
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_transaction_add_uninstall(m_transaction, refName.toUtf8().constData(), &localError)) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            setProgress(100);
            qWarning() << "Failed to uninstall" << refName << ':' << m_errorMessage;
            return;
        }
    }

    m_result = flatpak_transaction_run(m_transaction, m_cancellable, &localError);
    if (!m_result) {
        if (localError->code == FLATPAK_ERROR_REF_NOT_FOUND) {
            m_errorMessage = i18n("Could not find '%1' in '%2'; please make sure it's available.", refName, m_app->origin());
        } else {
            m_errorMessage = QString::fromUtf8(localError->message);
        }
#if defined(FLATPAK_LIST_UNUSED_REFS)
    } else {
        const auto installation = flatpak_transaction_get_installation(m_transaction);
        g_autoptr(GError) refsError = nullptr;
        g_autoptr(GPtrArray) refs = flatpak_installation_list_unused_refs(installation, nullptr, m_cancellable, &refsError);
        if (!refs) {
            qWarning() << "could not fetch unused refs" << refsError->message;
        } else if (refs->len > 0) {
            g_autoptr(GError) localError = nullptr;
            qDebug() << "found unused refs:" << refs->len;
            auto transaction = flatpak_transaction_new_for_installation(installation, m_cancellable, &localError);
            for (uint i = 0; i < refs->len; i++) {
                FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
                g_autofree gchar *strRef = flatpak_ref_format_ref(ref);
                qDebug() << "unused ref:" << strRef;
                if (!flatpak_transaction_add_uninstall(transaction, strRef, &localError)) {
                    qDebug() << "failed to uninstall unused ref" << refName << localError->message;
                    break;
                }
            }
            if (!flatpak_transaction_run(transaction, m_cancellable, &localError)) {
                qWarning() << "could not properly clean the elements" << refs->len << localError->message;
            }
        }
#endif
    }
    // We are done so we can set the progress to 100
    setProgress(100);
}

void FlatpakTransactionThread::setProgress(int progress)
{
    Q_ASSERT(qBound(0, progress, 100) == progress);
    if (m_progress != progress) {
        m_progress = progress;
        Q_EMIT progressChanged(m_progress);
    }
}

void FlatpakTransactionThread::setSpeed(quint64 speed)
{
    if (m_speed != speed) {
        m_speed = speed;
        Q_EMIT speedChanged(m_speed);
    }
}

QString FlatpakTransactionThread::errorMessage() const
{
    return m_errorMessage;
}

bool FlatpakTransactionThread::result() const
{
    return m_result;
}

void FlatpakTransactionThread::addErrorMessage(const QString &error)
{
    if (!m_errorMessage.isEmpty())
        m_errorMessage.append(QLatin1Char('\n'));
    m_errorMessage.append(error);
}

bool FlatpakTransactionThread::cancelled() const
{
    return g_cancellable_is_cancelled(m_cancellable);
}
