/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

#include "PopConParser.h"
#include "Rating.h"
#include <QDebug>
#include <QRegularExpression>

QHash<QString, Rating *> PopConParser::parsePopcon(QObject* parent, QIODevice* dev)
{
    QHash<QString, Rating *> ratings;
    QRegularExpression rx(QStringLiteral("^Package: ([^ ]+) +(\\d+) +(\\d+) +(\\d+) +(\\d+)\\s*$"));
    QString buff;
    buff.resize(512);
    QTextStream stream(dev);
    while(!stream.atEnd()) {
        stream.readLineInto(&buff, 0);

        auto match = rx.match(buff);
        if (!match.hasMatch())
            continue;

        const QString pkgName = match.captured(1);

        //according to popcon spec
        const int inst = match.capturedRef(2).toInt();
        const int vote = match.capturedRef(3).toInt();
        const int old = match.capturedRef(4).toInt();
        const int recent = match.capturedRef(5).toInt();

        Rating *rating = new Rating(pkgName, inst, vote, old, recent);
        if (!rating->ratingCount()) {
            delete rating;
            continue;
        }
        rating->setParent(parent);
        ratings[rating->packageName()] = rating;
    }

    return ratings;
}
