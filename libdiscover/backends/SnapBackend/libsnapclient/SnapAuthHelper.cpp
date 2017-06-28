/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <unistd.h>
#include <stdlib.h>
#include <kauthhelpersupport.h>
#include <kauthactionreply.h>
#include <Snapd/Client>

using namespace KAuth;

class SnapAuthHelper : public QObject
{
    Q_OBJECT
    QSnapdClient m_client;
public:
    SnapAuthHelper() {
        m_client.connect()->runAsync();
    }

public Q_SLOTS:
    ActionReply modify(const QVariantMap &args)
    {
        const QString user = args[QStringLiteral("user")].toString()
                    , pass = args[QStringLiteral("password")].toString()
                    , otp  = args[QStringLiteral("otp")].toString();

        QSnapdLoginRequest* req = otp.isEmpty() ? m_client.login(user, pass)
                                                : m_client.login(user, pass, otp);

        req->runSync();

        auto auth = req->authData();
        const QByteArray replyData = QJsonDocument(QJsonObject{
                {QStringLiteral("macaroon"), auth->macaroon()},
                {QStringLiteral("discharges"), QJsonArray::fromStringList(auth->discharges())}
            }).toJson();
        ActionReply reply = req->error() == QSnapdRequest::NoError ? ActionReply::SuccessReply() : ActionReply::InvalidActionReply();

        bool otpMode = req->error() == QSnapdConnectRequest::TwoFactorRequired;

        reply.setData({
            { QStringLiteral("reply"), replyData },
            { QStringLiteral("errorString"), req->errorString() },
            { QStringLiteral("otpMode"), otpMode }
        });
        return reply;
    }
};

KAUTH_HELPER_MAIN("org.kde.discover.libsnapclient", SnapAuthHelper)

#include "SnapAuthHelper.moc"
