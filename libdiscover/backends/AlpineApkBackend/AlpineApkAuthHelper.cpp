/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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
#include <QFile>
#include <KAuthHelperSupport>

#include "AlpineApkAuthHelper.h"

using namespace KAuth;

AlpineApkAuthHelper::AlpineApkAuthHelper() {}

ActionReply AlpineApkAuthHelper::test(const QVariantMap &args)
{
    const QString txt = args[QStringLiteral("txt")].toString();

    ActionReply reply = ActionReply::HelperErrorReply();
    QByteArray replyData(QByteArrayLiteral("ok"));

    QFile f(QStringLiteral("/lol.txt"));
    if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
        f.write(txt.toUtf8());
        f.close();

        reply = ActionReply::SuccessReply();
        reply.setData({
            { QStringLiteral("reply"), replyData },
        });
    }

    return reply;
}

KAUTH_HELPER_MAIN("org.kde.discover.alpineapkbackend", AlpineApkAuthHelper)
