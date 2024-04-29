/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakTransactionThread.h"
#include "FlatpakResource.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <KLocalizedString>
#include <QDesktopServices>

static int FLATPAK_CLI_UPDATE_FREQUENCY = 150;

gboolean FlatpakTransactionThread::add_new_remote_cb(FlatpakTransaction *object,
                                                     gint /*reason*/,
                                                     gchar *from_id,
                                                     gchar *suggested_remote_name,
                                                     gchar *url,
                                                     gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

    // TODO ask instead
    auto name = QString::fromUtf8(suggested_remote_name);
    obj->m_addedRepositories[FlatpakResource::installationPath(flatpak_transaction_get_installation(object))].append(name);
    Q_EMIT obj->passiveMessage(i18n("Adding remote '%1' in %2 from %3", name, QString::fromUtf8(url), QString::fromUtf8(from_id)));
    return true;
}

void FlatpakTransactionThread::progress_changed_cb(FlatpakTransactionProgress *progress, gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

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
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);

    g_signal_connect(progress, "changed", G_CALLBACK(&FlatpakTransactionThread::progress_changed_cb), obj);
    flatpak_transaction_progress_set_update_frequency(progress, FLATPAK_CLI_UPDATE_FREQUENCY);
}

void operation_error_cb(FlatpakTransaction * /*object*/, FlatpakTransactionOperation * /*operation*/, GError *error, gint /*details*/, gpointer user_data)
{
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
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
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "starting web flow" << webflowUrl << remote << id;
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
    obj->m_webflows << id;
    Q_EMIT obj->webflowStarted(webflowUrl, id);
    return true;
}

void FlatpakTransactionThread::webflowDoneCallback(FlatpakTransaction *transaction, GVariant *options, guint id, gpointer user_data)
{
    Q_UNUSED(transaction);
    Q_UNUSED(options);
    auto obj = static_cast<FlatpakTransactionThread *>(user_data);
    obj->m_webflows << id;
    Q_EMIT obj->webflowDone(id);
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "webflow done" << id;
}

namespace Callbacks
{
void operation_done([[maybe_unused]] FlatpakTransaction *transaction,
                    [[maybe_unused]] FlatpakTransactionOperation *operation,
                    const char *commit,
                    [[maybe_unused]] FlatpakTransactionResult details)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << commit;
}

int choose_remote_for_ref([[maybe_unused]] FlatpakTransaction *transaction, const char *for_ref, const char *runtime_ref, const char *const *remotes)
{
    const auto remotesCount = g_strv_length(const_cast<char **>(remotes));
    if (LIBDISCOVER_BACKEND_FLATPAK_LOG().isDebugEnabled()) {
        const auto remotesSpan = std::span{remotes, remotesCount};
        QStringList remotesStrings;
        remotesStrings.reserve(remotesCount);
        for (const auto &remote : remotesSpan) {
            remotesStrings.append(QString::fromUtf8(remote));
        }
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << for_ref << runtime_ref << remotesStrings;
    }
    // NOTE: this shouldn't actually happen for us, we always know the remote beforehand. If we fail the assertion here that indicates we have
    // a problem earlier in the workflow producing multiple (unexpected) remotes.
    Q_ASSERT(remotesCount <= 1);
    return 0;
}

void end_of_lifed([[maybe_unused]] FlatpakTransaction *transaction, const char *ref, const char *reason, const char *rebase)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << ref << reason << rebase;
}

gboolean ready([[maybe_unused]] FlatpakTransaction *transaction)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}

gboolean run([[maybe_unused]] FlatpakTransaction *transaction, [[maybe_unused]] GCancellable *cancellable, [[maybe_unused]] GError **error)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}

gboolean end_of_lifed_with_rebase([[maybe_unused]] FlatpakTransaction *transaction,
                                  const char *remote,
                                  const char *ref,
                                  const char *reason,
                                  const char *rebased_to_ref,
                                  const char **previous_ids,
                                  gpointer user_data)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "end_of_lifed_with_rebase" << remote << ref << reason << rebased_to_ref << previous_ids;
    return false;
}

gboolean basic_auth_start([[maybe_unused]] FlatpakTransaction *transaction,
                          const char *remote,
                          const char *realm,
                          [[maybe_unused]] GVariant *options,
                          [[maybe_unused]] guint id)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << remote << realm;
    return true; // continue with transaction
}

void install_authenticator([[maybe_unused]] FlatpakTransaction *transaction, const char *remote, const char *authenticator_ref)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP" << remote << authenticator_ref;
}

gboolean ready_pre_auth([[maybe_unused]] FlatpakTransaction *transaction)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "NOOP";
    return true; // continue with transaction
}
} // namespace Callbacks

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
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to create transaction" << m_errorMessage;
    } else {
        g_signal_connect(m_transaction, "add-new-remote", G_CALLBACK(add_new_remote_cb), this);
        g_signal_connect(m_transaction, "new-operation", G_CALLBACK(new_operation_cb), this);
        g_signal_connect(m_transaction, "operation-error", G_CALLBACK(operation_error_cb), this);

        g_signal_connect(m_transaction, "basic-auth-start", G_CALLBACK(Callbacks::basic_auth_start), this);
        g_signal_connect(m_transaction, "choose-remote-for-ref", G_CALLBACK(Callbacks::choose_remote_for_ref), this);
        g_signal_connect(m_transaction, "end-of-lifed", G_CALLBACK(Callbacks::end_of_lifed), this);
        g_signal_connect(m_transaction, "end-of-lifed-with-rebase", G_CALLBACK(Callbacks::end_of_lifed_with_rebase), this);
        g_signal_connect(m_transaction, "install-authenticator", G_CALLBACK(Callbacks::install_authenticator), this);
        g_signal_connect(m_transaction, "operation-done", G_CALLBACK(Callbacks::operation_done), this);
        g_signal_connect(m_transaction, "ready", G_CALLBACK(Callbacks::ready), this);
        g_signal_connect(m_transaction, "ready-pre-auth", G_CALLBACK(Callbacks::ready_pre_auth), this);

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
    for (int id : std::as_const(m_webflows)) {
        flatpak_transaction_abort_webflow(m_transaction, id);
    }
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionThread::run()
{
    auto finish = qScopeGuard([this] {
        Q_EMIT finished();
    });

    if (!m_transaction) {
        return;
    }
    g_autoptr(GError) localError = nullptr;

    const QString refName = m_app->ref();

    if (m_role == Transaction::Role::InstallRole) {
        bool correct = false;
        if (m_app->state() == AbstractResource::Upgradeable && m_app->isInstalled()) {
            correct = flatpak_transaction_add_update(m_transaction, refName.toUtf8().constData(), nullptr, nullptr, &localError);
            for (const QByteArray &subref : m_app->toUpdate()) {
                correct |= flatpak_transaction_add_update(m_transaction, subref.constData(), nullptr, nullptr, &localError);
            }
        } else if (m_app->flatpakFileType() == FlatpakResource::FileFlatpak) {
            g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
            if (!file) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install bundled application" << refName;
                m_result = false;
                return;
            }
            correct = flatpak_transaction_add_install_bundle(m_transaction, file, nullptr, &localError);
        } else if (m_app->flatpakFileType() == FlatpakResource::FileFlatpakRef && m_app->resourceFile().isLocalFile()) {
            g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
            if (!file) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install flatpakref application" << refName;
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
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to install" << m_app->flatpakFileType() << refName << ':' << m_errorMessage;
            return;
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_transaction_add_uninstall(m_transaction, refName.toUtf8().constData(), &localError)) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            setProgress(100);
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to uninstall" << refName << ':' << m_errorMessage;
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
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not fetch unused refs" << refsError->message;
        } else if (refs->len > 0) {
            g_autoptr(GError) localError = nullptr;
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "found unused refs:" << refs->len;
            auto transaction = flatpak_transaction_new_for_installation(installation, m_cancellable, &localError);
            for (uint i = 0; i < refs->len; i++) {
                FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
                g_autofree gchar *strRef = flatpak_ref_format_ref(ref);
                qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "unused ref:" << strRef;
                if (!flatpak_transaction_add_uninstall(transaction, strRef, &localError)) {
                    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to uninstall unused ref" << refName << localError->message;
                    break;
                }
            }
            if (!flatpak_transaction_run(transaction, m_cancellable, &localError)) {
                qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not properly clean the elements" << refs->len << localError->message;
            }
        }
#endif
    }
    // We are done so we can set the progress to 100
    setProgress(100);
}

int FlatpakTransactionThread::progress() const
{
    return m_progress;
}

void FlatpakTransactionThread::setProgress(int progress)
{
    Q_ASSERT(qBound(0, progress, 100) == progress);
    if (m_progress > progress) {
        qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "progress is regressing :(" << m_progress << progress;
        return;
    }

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
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.append(QLatin1Char('\n'));
    }
    m_errorMessage.append(error);
}

bool FlatpakTransactionThread::cancelled() const
{
    return g_cancellable_is_cancelled(m_cancellable);
}

FlatpakTransactionThread::Repositories FlatpakTransactionThread::addedRepositories() const
{
    return m_addedRepositories;
}

#include "moc_FlatpakTransactionThread.cpp"
