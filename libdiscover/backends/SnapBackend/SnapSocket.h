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

#ifndef SNAPSOCKET_H
#define SNAPSOCKET_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocalSocket>

class SnapJob : public QObject
{
    Q_OBJECT
public:
    SnapJob(QObject* parent = nullptr);

    QJsonValue result() const { return m_data.value(QLatin1String("result")); }
    int statusCode() const { return m_data.value(QLatin1String("status-code")).toInt(); }
    QString status() const { return m_data.value(QLatin1String("status")).toString(); }
    QString type() const { return m_data.value(QLatin1String("type")).toString(); }

    void processReply(QIODevice* device);

    bool isSuccessful() const { return statusCode()==200; }

Q_SIGNALS:
    void finished();

private:
    QJsonObject m_data;
};

class SnapSocket : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)
public:
    SnapSocket(QObject* parent = nullptr);
    ~SnapSocket();

    bool isConnected() const;

    /// /v2/snaps query
    QJsonArray snaps();

    /// /v2/snaps/@p name
    QJsonObject snapByName(const QByteArray& name);

Q_SIGNALS:
    bool connectedChanged(bool connected);

private:
    QByteArray createRequest(const QByteArray &method, const QByteArray &path, const QByteArray &content) const;
    void stateChanged(QLocalSocket::LocalSocketState newState);

    QLocalSocket * const m_socket;
    QVector<SnapJob*> m_jobs;

    QByteArray m_macaroon;
    QJsonArray m_discharges;
};

#endif // SNAPSOCKET_H
