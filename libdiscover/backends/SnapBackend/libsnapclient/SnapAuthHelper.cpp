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
#include <QLocalSocket>
#include <unistd.h>
#include <stdlib.h>
#include <kauthhelpersupport.h>
#include <kauthactionreply.h>

using namespace KAuth;

class SnapAuthHelper : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    ActionReply modify(const QVariantMap &args)
    {
        QLocalSocket socket;
        socket.connectToServer(QStringLiteral("/run/snapd.socket"), QIODevice::ReadWrite);
        const bool b = socket.waitForConnected();
        Q_ASSERT(b);

        const QByteArray request = args[QStringLiteral("request")].toByteArray();
        socket.write(request);
        socket.waitForReadyRead();
        const auto replyData = socket.readAll();

        ActionReply reply = ActionReply::SuccessReply();
        reply.setData({ { QStringLiteral("reply"), replyData } });
        return reply;
    }
};

KAUTH_HELPER_MAIN("org.kde.discover.libsnapclient", SnapAuthHelper)

#include "SnapAuthHelper.moc"
