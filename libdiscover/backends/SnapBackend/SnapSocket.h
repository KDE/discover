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

class QUrlQuery;

class SnapJob : public QObject
{
    Q_OBJECT
public:
    SnapJob(const QByteArray& request, QObject* parent = nullptr);

    QJsonValue result() const { return m_data.value(QLatin1String("result")); }
    int statusCode() const { return m_data.value(QLatin1String("status-code")).toInt(); }
    QString status() const { return m_data.value(QLatin1String("status")).toString(); }
    QString type() const { return m_data.value(QLatin1String("type")).toString(); }

    bool exec();

    bool isSuccessful() const { return statusCode()==200; }

Q_SIGNALS:
    void finished();

private:
    void processReply();

    QLocalSocket * const m_socket;
    QJsonObject m_data;
};

/**
 * https://developer.ubuntu.com/en/snappy/guides/rest/
 */

class SnapSocket : public QObject
{
    Q_OBJECT
//     Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)
public:
    SnapSocket(QObject* parent = nullptr);
    ~SnapSocket();

//     bool isConnected() const;

    /// GET /v2/snaps
    SnapJob* snaps();

    /// GET /v2/snaps/@p name
    SnapJob* snapByName(const QByteArray& name);

    /// GET /v2/find query
    SnapJob* find(const QString &query);

    /// GET /v2/find query
    SnapJob* findByName(const QString &name);

    enum SnapAction { Install, Refresh, Remove, Revert, Enable, Disable };

    /// POST /v2/snaps/@p name
    /// stable is the default channel
    SnapJob* snapAction(const QString &name, SnapAction action, const QString &channel = {});

// Q_SIGNALS:
//     bool connectedChanged(bool connected);

private:
    QByteArray createRequest(const QByteArray &method, const QByteArray &path, const QUrlQuery &content) const;
    QByteArray createRequest(const QByteArray &method, const QByteArray &path, const QByteArray &content = {}) const;

    QByteArray m_macaroon;
    QJsonArray m_discharges;
};

#endif // SNAPSOCKET_H
