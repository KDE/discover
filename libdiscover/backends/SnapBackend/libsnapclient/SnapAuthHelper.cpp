/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KAuthActionReply>
#include <KAuthHelperSupport>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <Snapd/Client>
#include <stdlib.h>
#include <unistd.h>

using namespace KAuth;

class SnapAuthHelper : public QObject
{
    Q_OBJECT
    QSnapdClient m_client;

public:
    SnapAuthHelper()
    {
    }

public Q_SLOTS:
    KAuth::ActionReply login(const QVariantMap &args)
    {
        const QString user = args[QStringLiteral("user")].toString(), pass = args[QStringLiteral("password")].toString(),
                      otp = args[QStringLiteral("otp")].toString();

        QScopedPointer<QSnapdLoginRequest> req(otp.isEmpty() ? m_client.login(user, pass) : m_client.login(user, pass, otp));

        req->runSync();

        ActionReply reply;
        bool otpMode = false;
        QByteArray replyData;

        if (req->error() == QSnapdRequest::NoError) {
            const auto auth = req->authData();
            replyData = QJsonDocument(QJsonObject{{QStringLiteral("macaroon"), auth->macaroon()},
                                                  {QStringLiteral("discharges"), QJsonArray::fromStringList(auth->discharges())}})
                            .toJson();

            reply = ActionReply::SuccessReply();
        } else {
            otpMode = req->error() == QSnapdConnectRequest::TwoFactorRequired;
            reply = ActionReply::InvalidActionReply();
            reply.setErrorDescription(req->errorString());
        }
        reply.setData({{QStringLiteral("reply"), replyData}, {QStringLiteral("otpMode"), otpMode}});
        return reply;
    }
};

KAUTH_HELPER_MAIN("org.kde.discover.libsnapclient", SnapAuthHelper)

#include "SnapAuthHelper.moc"
