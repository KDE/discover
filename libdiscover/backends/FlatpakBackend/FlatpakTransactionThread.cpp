/***************************************************************************
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "FlatpakTransactionThread.h"
#include "FlatpakResource.h"

#include <KLocalizedString>
#include <QDebug>

static int FLATPAK_CLI_UPDATE_FREQUENCY = 150;

gboolean
add_new_remote_cb(FlatpakTransaction */*object*/,
               gint                /*reason*/,
               gchar              *from_id,
               gchar              *suggested_remote_name,
               gchar              *url,
               gpointer            user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread*) user_data;

    //TODO ask instead
    Q_EMIT obj->passiveMessage(i18n("Adding remote '%1' in %2 from %3", QString::fromUtf8(suggested_remote_name), QString::fromUtf8(url), QString::fromUtf8(from_id)));
    return true;
}

static void
progress_changed_cb (FlatpakTransactionProgress *progress,
                     gpointer                    user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread*) user_data;

    qDebug() << "progress" << flatpak_transaction_progress_get_progress(progress);
    obj->setProgress(flatpak_transaction_progress_get_progress(progress));
}

void
new_operation_cb(FlatpakTransaction          */*object*/,
               FlatpakTransactionOperation */*operation*/,
               FlatpakTransactionProgress  *progress,
               gpointer                     user_data)
{
    FlatpakTransactionThread *obj = (FlatpakTransactionThread*) user_data;


    g_signal_connect (progress, "changed", G_CALLBACK (progress_changed_cb), obj);
    flatpak_transaction_progress_set_update_frequency (progress, FLATPAK_CLI_UPDATE_FREQUENCY);
}

FlatpakTransactionThread::FlatpakTransactionThread(FlatpakResource *app, Transaction::Role role)
    : QThread()
    , m_result(false)
    , m_progress(0)
    , m_app(app)
    , m_role(role)
{
    m_cancellable = g_cancellable_new();

    g_autoptr(GError) localError = nullptr;
    m_transaction = flatpak_transaction_new_for_installation(m_app->installation(), m_cancellable, &localError);
    g_signal_connect (m_transaction, "add-new-remote", G_CALLBACK (add_new_remote_cb), this);
    g_signal_connect (m_transaction, "new-operation", G_CALLBACK (new_operation_cb), this);
}

FlatpakTransactionThread::~FlatpakTransactionThread()
{
    g_object_unref(m_transaction);
    g_object_unref(m_cancellable);
}

void FlatpakTransactionThread::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionThread::run()
{
    g_autoptr(GError) localError = nullptr;

    const QString refName = m_app->ref();

    bool correct = false;
    if (m_role == Transaction::Role::InstallRole) {
        if (m_app->state() == AbstractResource::Upgradeable && m_app->isInstalled()) {
            correct = flatpak_transaction_add_update(m_transaction,
                                              refName.toUtf8().constData(),
                                              nullptr, nullptr, &localError);
        } else {
            if (m_app->flatpakFileType() == QStringLiteral("flatpak")) {
                g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
                if (!file) {
                    qWarning() << "Failed to install bundled application" << refName;
                    m_result = false;
                    return;
                }
                correct = flatpak_transaction_add_install_bundle(m_transaction, file, nullptr, &localError);
            } else {
                correct = flatpak_transaction_add_install(m_transaction,
                                                   m_app->origin().toUtf8().constData(),
                                                   refName.toUtf8().constData(),
                                                   nullptr,
                                                   &localError);
            }
        }

        if (!correct) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            setProgress(100);
            qWarning() << "Failed to install" << refName << ':' << m_errorMessage;
            return;
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_transaction_add_uninstall(m_transaction,
                                            refName.toUtf8().constData(),
                                            &localError)) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            setProgress(100);
            qWarning() << "Failed to uninstall" << refName << ':' << m_errorMessage;
            return;
        }
    }

    // We are done so we can set the progress to 100
    m_result = flatpak_transaction_run(m_transaction, m_cancellable, &localError);
    setProgress(100);
}

FlatpakResource * FlatpakTransactionThread::app() const
{
    return m_app;
}

int FlatpakTransactionThread::progress() const
{
    return m_progress;
}

void FlatpakTransactionThread::setProgress(int progress)
{
    if (m_progress != progress) {
        m_progress = progress;
        Q_EMIT progressChanged(m_progress);
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

