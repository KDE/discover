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
#include <QRegularExpression>

SnapSocket::SnapSocket(QObject* parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::connected, this, [](){ qDebug() << "connected"; });
    connect(m_socket, &QLocalSocket::disconnected, this, [](){ qDebug() << "disconnected :("; });
    connect(m_socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, [](QLocalSocket::LocalSocketError socketError){ qDebug() << "error!" << socketError; });

    m_socket->connectToServer(QStringLiteral("/run/snapd.socket"), QIODevice::ReadWrite);
}

SnapSocket::~SnapSocket()
{
    qDeleteAll(m_jobs);
}

bool SnapSocket::isConnected() const
{
    return m_socket->isOpen();
}

void SnapSocket::stateChanged(QLocalSocket::LocalSocketState newState)
{
//     qDebug() << "state changed!" << newState;
    switch(newState) {
        case QLocalSocket::ConnectedState:
            Q_EMIT connectedChanged(true);
            break;
        case QLocalSocket::UnconnectedState:
            Q_EMIT connectedChanged(false);
            break;
        default:
            break;
    }
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
        request += "r\n";
        request += content;
    } else {
        request += "\r\n";
    }
//     qDebug() << "request" << request;
    return request;
}

QJsonArray SnapSocket::snaps()
{
    auto sj = new SnapJob();
    m_jobs.append(sj);
    m_socket->write(createRequest("GET", "/v2/snaps", {}));
    m_socket->waitForReadyRead();
    sj->processReply(m_socket);

    return sj->isSuccessful() ? sj->result().toArray() : QJsonArray{};
}

QJsonObject SnapSocket::snapByName(const QByteArray& name)
{
    auto sj = new SnapJob();
    m_jobs.append(sj);
    m_socket->write(createRequest("GET", "/v2/snaps/"+name, {}));
    m_socket->waitForReadyRead();
    sj->processReply(m_socket);

    return sj->isSuccessful() ? sj->result().toObject() : QJsonObject{};
}

QJsonArray SnapSocket::find(const QString& query)
{
    auto sj = new SnapJob();
    m_jobs.append(sj);
    m_socket->write(createRequest("GET", "/v2/find?q="+query.toUtf8(), {}));
    m_socket->waitForReadyRead();
    sj->processReply(m_socket);

    return sj->isSuccessful() ? sj->result().toArray() : QJsonArray{};
}

QJsonArray SnapSocket::findByName(const QString& name)
{
    return {};
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
        if (!line.endsWith("OK"))
            qWarning() << "error" << line;
    }

    QHash<QByteArray, QByteArray> headers;
    for(; true; ) {
        const QByteArray line = device->readLine();
        if (line == "\r\n") {
            break;
        }

        const auto idx = line.indexOf(": ");
        if (idx<=0) {
            qWarning() << "error: wrong header" << line;
        }
        const auto id = line.left(idx);
        const auto val = line.mid(idx+2).trimmed();
        headers[id] = val;
    }

    const auto transferEncoding = headers.value("Transfer-Encoding");
    const bool chunked = transferEncoding == "chunked";
    qint64 length = 0;
    if (!chunked) {
        const auto numberStr = headers.value("Content-Length");
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
                const auto number = numberString.toInt(&ok, 16);

                if (number == 0)
                    break;
                rest += device->read(number);
                device->read(2);
            }
            device->read(2);
        } else {
            rest = device->read(length);
        }
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(rest, &error);
        if (error.error)
            qWarning() << "error parsing json" << error.errorString();
        if (!doc.isObject())
            qWarning() << "wrong object type" << rest;
        m_data = doc.object();
    }
}
