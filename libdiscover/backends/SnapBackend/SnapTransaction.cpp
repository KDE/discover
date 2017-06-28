/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "SnapTransaction.h"
#include "SnapBackend.h"
#include "SnapResource.h"
#include <Snapd/Request>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include "libsnapclient/config-paths.h"
#include "utils.h"

SnapTransaction::SnapTransaction(SnapResource* app, QSnapdRequest* request, Role role)
    : Transaction(app, app, role)
    , m_app(app)
    , m_request(request)
{
    setStatus(DownloadingStatus);
    setCancellable(false);
    connect(request, &QSnapdRequest::progress, this, &SnapTransaction::progressed);
    connect(request, &QSnapdRequest::complete, this, &SnapTransaction::finishTransaction);
    setStatus(SetupStatus);

    request->runAsync();
}

void SnapTransaction::cancel()
{
    m_request->cancel();
}

void SnapTransaction::finishTransaction()
{
    switch(m_request->error()) {
        case QSnapdRequest::NoError:
            static_cast<SnapBackend*>(m_app->backend())->refreshStates();
            break;
        case QSnapdRequest::AuthDataRequired: {
            QProcess* p = new QProcess;
            p->setProgram(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR "/discover/SnapMacaroonDialog"));
            p->start();

            connect(p, static_cast<void(QProcess::*)(int)>(&QProcess::finished), this, [this, p] (int code) {
                p->deleteLater();
                if (code != 0) {
                    qWarning() << "login failed..." << p->readAll();
                    Q_EMIT passiveMessage(m_request->errorString());
                    return;
                }
                const auto doc = QJsonDocument::fromJson(p->readAllStandardOutput());
                const auto result = doc.object();

                const auto macaroon = result[QStringLiteral("macaroon")].toString();
                const auto discharges = kTransform<QStringList>(result[QStringLiteral("discharges")].toArray(), [](const QJsonValue& val) { return val.toString(); });
                static_cast<SnapBackend*>(m_app->backend())->client()->setAuthData(new QSnapdAuthData(macaroon, discharges));
                m_request->runAsync();
            });
        }   return;
        default:
            Q_EMIT passiveMessage(m_request->errorString());
            break;
    }

    setStatus(DoneStatus);
}

void SnapTransaction::progressed()
{
//     setProgress(m_request->change()->???);
}
