/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SnapTransaction.h"
#include "SnapBackend.h"
#include "SnapResource.h"
#include "libsnapclient/config-paths.h"
#include "utils.h"
#include <KLocalizedString>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>
#include <Snapd/Request>

SnapTransaction::SnapTransaction(QSnapdClient *client, SnapResource *app, Role role, AbstractResource::State newState)
    : Transaction(app, app, role)
    , m_client(client)
    , m_app(app)
    , m_newState(newState)
{
    if (role == RemoveRole)
        setRequest(m_client->remove(app->packageName()));
    else if (app->state() == AbstractResource::Upgradeable)
        setRequest(m_client->refresh(app->packageName()));
    else
        setRequest(m_client->install(app->packageName()));
}

void SnapTransaction::cancel()
{
    m_request->cancel();
}

void SnapTransaction::finishTransaction()
{
    switch (m_request->error()) {
    case QSnapdRequest::NoError:
        static_cast<SnapBackend *>(m_app->backend())->refreshStates();
        setStatus(DoneStatus);
        m_app->setState(m_newState);
        break;
    case QSnapdRequest::Cancelled:
        setStatus(CancelledStatus);
        break;
    case QSnapdRequest::NeedsClassic:
        setStatus(SetupStatus);
        if (role() == Transaction::InstallRole) {
            Q_EMIT proceedRequest(m_app->name(),
                                  i18n("This Snap application is not compatible with security sandboxing "
                                       "and will have full access to this computer. Install it anyway?"));
            return;
        }
        break;
    case QSnapdRequest::AuthDataRequired: {
        setStatus(SetupStatus);
        QProcess *p = new QProcess;
        p->setProgram(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR "/discover/SnapMacaroonDialog"));
        p->start();

        connect(p, &QProcess::finished, this, [this, p](int code, QProcess::ExitStatus status) {
            p->deleteLater();
            if (code != 0) {
                qWarning() << "login failed... code:" << code << status << p->readAll();
                Q_EMIT passiveMessage(m_request->errorString());
                setStatus(DoneWithErrorStatus);
                return;
            }
            const auto doc = QJsonDocument::fromJson(p->readAllStandardOutput());
            const auto result = doc.object();

            const auto macaroon = result[QStringLiteral("macaroon")].toString();
            const auto discharges = kTransform<QStringList>(result[QStringLiteral("discharges")].toArray(), [](const QJsonValue &val) {
                return val.toString();
            });
            static_cast<SnapBackend *>(m_app->backend())->client()->setAuthData(new QSnapdAuthData(macaroon, discharges));
            m_request->runAsync();
        });
    }
        return;
    default:
        qDebug() << "snap error" << m_request.get() << m_request->error() << m_request->errorString();
        Q_EMIT passiveMessage(m_request->errorString());
        setStatus(DoneWithErrorStatus);
        break;
    }
}

void SnapTransaction::proceed()
{
    setRequest(m_client->install(QSnapdClient::Classic, m_app->packageName()));
}

void SnapTransaction::setRequest(QSnapdRequest *req)
{
    m_request.reset(req);

    setCancellable(true);
    connect(m_request.data(), &QSnapdRequest::progress, this, &SnapTransaction::progressed);
    connect(m_request.data(), &QSnapdRequest::complete, this, &SnapTransaction::finishTransaction);

    setStatus(CommittingStatus);
    m_request->runAsync();
}

void SnapTransaction::progressed()
{
    const auto change = m_request->change();
    int percentage = 0;
    int count = 0;

    auto status = SetupStatus;
    quint64 bitsPerSecond = 0;
    for (int i = 0, c = change->taskCount(); i < c; ++i) {
        ++count;
        auto task = change->task(i);
        if (task->kind() == QLatin1String("download-snap")) {
            status = task->status() == QLatin1String("doing") || task->status() == QLatin1String("do") ? DownloadingStatus : CommittingStatus;
        } else if (task->kind() == QLatin1String("clear-snap")) {
            status = CommittingStatus;
        }
        percentage += (100 * task->progressDone()) / task->progressTotal();

        if (const auto readyTime = task->readyTime(); readyTime.isValid()) {
            const auto now = QDateTime::currentDateTime();
            const auto timeDelta = now - readyTime;
            const auto timeDeltaSeconds = std::chrono::duration_cast<std::chrono::seconds>(timeDelta).count();
            const auto downloadedBits = (task->progressTotal() - task->progressDone()) * 8;
            qDebug() << timeDeltaSeconds << downloadedBits;
            bitsPerSecond = timeDeltaSeconds <= 0 ? 0 : downloadedBits / timeDeltaSeconds;
            qDebug() << bitsPerSecond;
        }
    }
    setProgress(percentage / qMax(count, 1));
    setStatus(status);
    setDownloadSpeed(bitsPerSecond);
}

#include "moc_SnapTransaction.cpp"
