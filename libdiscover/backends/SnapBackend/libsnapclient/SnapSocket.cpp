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

#include "SnapSocket.h"
#include <QDebug>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QBuffer>
#include <QRegularExpression>
#include <kauthexecutejob.h>

class LocalSnapJob : public SnapJob
{
public:
    LocalSnapJob(const QByteArray& request, QObject* parent = nullptr)
        : SnapJob(parent)
        , m_socket(new QLocalSocket(this))
    {
        connect(m_socket, &QLocalSocket::connected, this, [this, request](){
//             qDebug() << "connected" << request;
            m_socket->write(request);
        });
        connect(m_socket, &QLocalSocket::disconnected, this, [](){ qDebug() << "disconnected :("; });
        connect(m_socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, [](QLocalSocket::LocalSocketError socketError){ qDebug() << "error!" << socketError; });
        connect(m_socket, &QLocalSocket::readyRead, this, [this](){ processReply(m_socket); });

        m_socket->connectToServer(QStringLiteral("/run/snapd.socket"), QIODevice::ReadWrite);
    }

    bool exec() override
    {
        m_socket->waitForReadyRead();
        return isSuccessful();
    }

private:
    QLocalSocket * const m_socket;
};

class AuthSnapJob : public SnapJob
{
public:
    AuthSnapJob(const QByteArray& request, QObject* parent = nullptr)
        : SnapJob(parent)
    {
        KAuth::Action snapAction(QStringLiteral("org.kde.discover.libsnapclient.modify"));
        snapAction.setHelperId(QStringLiteral("org.kde.discover.libsnapclient"));
        qDebug() << "requesting through kauth" << request;
        snapAction.setArguments({ { QStringLiteral("request"), request } });
        Q_ASSERT(snapAction.isValid());
        m_reply = snapAction.execute();
        m_reply->start();

        connect(m_reply, &KAuth::ExecuteJob::finished, this, &AuthSnapJob::authJobFinished);
    }

    bool exec() override
    {
        Q_UNIMPLEMENTED();
//         m_reply->exec();
        return isSuccessful();
    }

private:
    void authJobFinished()
    {
        if (m_reply->error() == KJob::NoError) {
            const auto reply = m_reply->data()[QStringLiteral("reply")].toByteArray();
            QBuffer buffer;
            buffer.setData(reply);
            qDebug() << "replied!" << reply;

            bool b = buffer.open(QIODevice::ReadOnly);
            Q_ASSERT(b);
            processReply(&buffer);
        } else {
            qDebug() << "kauth error" << m_reply->error() << m_reply->errorString();
            Q_EMIT finished(this);
        }
    }

    KAuth::ExecuteJob* m_reply = nullptr;
};

SnapSocket::SnapSocket(QObject* parent)
    : QObject(parent)
{
}

SnapSocket::~SnapSocket()
{
}

QByteArray SnapSocket::createRequest(const QByteArray& method, const QByteArray& path, const QJsonObject& content) const
{
    QByteArray ret;
    if (method == "GET") {
        QUrlQuery uq;
        for(auto it = content.constBegin(), itEnd = content.constEnd(); it!=itEnd; ++it) {
            uq.addQueryItem(it.key(), it.value().toString());
        }
        const auto query = uq.toString().toUtf8();
        ret = createRequest(method, path+'?'+query, QByteArray());
    } else if(method == "POST")
        ret = createRequest(method, path, QJsonDocument(content).toJson());
    else
        qWarning() << "unknown method" << method;
    return ret;
}

QByteArray SnapSocket::createRequest(const QByteArray &method, const QByteArray &path, const QByteArray &content) const
{
    QByteArray request = method + ' ' + path + " HTTP/1.1\r\n"
                         "Host:\r\n";
    if (!m_macaroon.isEmpty()) {
        request += "Authorization: Macaroon root=" + m_macaroon;
        for (const QJsonValue &val : m_discharges)
            request += ",discharge=\"" + val.toString().toLatin1() + '\"';
        request += "\r\n";
    }
    if (!content.isEmpty()) {
        request += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
        request += "\r\n";
        request += content;
    } else {
        request += "\r\n";
    }
//     qDebug() << "request" << request;
    return request;
}

SnapJob* SnapSocket::snaps()
{
    return new LocalSnapJob(createRequest("GET", "/v2/snaps"), this);
}

SnapJob* SnapSocket::snapByName(const QString& name)
{
    return new LocalSnapJob(createRequest("GET", "/v2/snaps/"+name.toUtf8()), this);
}

SnapJob * SnapSocket::find(SnapSocket::Select select)
{
    return new LocalSnapJob(createRequest("GET", "/v2/find", {{ QStringLiteral("select"), select==SelectRefresh ? QStringLiteral("refresh") : QStringLiteral("private") }}), this);
}

SnapJob* SnapSocket::find(const QString& query)
{
    return new LocalSnapJob(createRequest("GET", "/v2/find", {{ QStringLiteral("q"), query }}), this);
}

SnapJob* SnapSocket::findByName(const QString& name)
{
    return new LocalSnapJob(createRequest("GET", "/v2/find", {{ QStringLiteral("name"), name }}), this);
}

SnapJob * SnapSocket::changes(const QString& id)
{
    Q_ASSERT(!id.isEmpty());
    return new LocalSnapJob(createRequest("GET", "/v2/changes/"+id.toUtf8()), this);
}

SnapJob * SnapSocket::snapAction(const QString& name, SnapSocket::SnapAction action, const QString& channel)
{
    QString actionStr;
    switch(action) {
        case Install: actionStr = QStringLiteral("install"); break;
        case Refresh: actionStr = QStringLiteral("refresh"); break;
        case Remove: actionStr = QStringLiteral("remove"); break;
        case Revert: actionStr = QStringLiteral("revert"); break;
        case Enable: actionStr = QStringLiteral("enable"); break;
        case Disable: actionStr = QStringLiteral("disable"); break;
        default:
            Q_UNREACHABLE();
    }
    QJsonObject query = {{ QStringLiteral("action"), actionStr }};
    if (!channel.isEmpty())
        query.insert(QStringLiteral("channel"), channel);
    return new AuthSnapJob(createRequest("POST", "/v2/snaps/"+name.toUtf8(), query), this);
}

void SnapSocket::login(const QString& username, const QString& password, const QString& otp)
{
    QJsonObject obj = { { QStringLiteral("username"), username}, { QStringLiteral("password"), password } };
    if (!otp.isEmpty())
        obj.insert(QStringLiteral("obj"), obj);

    auto job = new LocalSnapJob(createRequest("POST", "/v2/login", obj), this);
    connect(job, &SnapJob::finished, this, &SnapSocket::includeCredentials);
}

void SnapSocket::includeCredentials(SnapJob* job)
{
    const auto result = job->result().toObject();
    m_macaroon = result[QStringLiteral("macaroon")].toString().toUtf8();
    m_discharges = result[QStringLiteral("discharges")].toArray();
}

/////////////

SnapJob::SnapJob(QObject* parent)
    : QObject(parent)
{
    connect(this, &SnapJob::finished, this, &QObject::deleteLater);
}

void SnapJob::processReply(QIODevice* device)
{
    {
        const QByteArray line = device->readLine().trimmed();
        if (!line.endsWith("OK") && !line.endsWith("Accepted"))
            qWarning() << "error" << line;
    }

    QHash<QByteArray, QByteArray> headers;
    for(; device->canReadLine(); ) {
        const QByteArray line = device->readLine();
        if (line == "\r\n") {
            break;
        }

        const auto idx = line.indexOf(": ");
        if (idx<=0) {
            qWarning() << "error: wrong header" << line;
        }
        const auto id = line.left(idx).toLower();
        const auto val = line.mid(idx+2).trimmed();
        headers[id] = val;
    }

    const auto transferEncoding = headers.value("transfer-encoding");
    const bool chunked = transferEncoding == "chunked";
    qint64 length = 0;
    if (!chunked) {
        const auto numberStr = headers.value("content-length");
        if (numberStr.isEmpty()) {
            qWarning() << "no content length" << headers;
            return;
        }

        bool ok;
        length = numberStr.toInt(&ok);
        if (!ok) {
            qWarning() << "wrong Content-Length integer parsing" << numberStr << headers;
            return;
        }
    }

    {
        QByteArray rest;
        if (chunked) {
            for (;;) {
                bool ok;
                const auto numberString = device->readLine().trimmed();
                auto number = numberString.toInt(&ok, 16);

                if (number == 0)
                    break;

                for (; number > 0; ) {
                    auto fu = device->read(number);
                    rest += fu;
                    number -= fu.size();
                     //TODO: split: payload retrieval and json processing so we don't block
                    if (number>0)
                        device->waitForReadyRead(5);
                }
                device->read(2);
            }
        } else {
            rest = device->read(length);
            Q_ASSERT(rest.size() == length);
        }
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(rest, &error);
        if (error.error)
            qWarning() << "error parsing json" << error.errorString() << device->bytesAvailable() << "..." << rest.right(10);
        else if (!doc.isObject())
            qWarning() << "wrong object type" << rest;
        m_data = doc.object();
    }
    Q_EMIT finished(this);
}
