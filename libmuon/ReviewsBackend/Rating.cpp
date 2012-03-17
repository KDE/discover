/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "Rating.h"
#include <QStringList>
#include <QDebug>

Rating::Rating(const QVariantMap &data)
{
    m_packageName = data.value("package_name").toString();
    m_appName = data.value("app_name").toString();
    m_ratingCount = data.value("ratings_total").toULongLong();

    QString ratingString = data.value("ratings_average").toString();
    m_rating = ratingString.toDouble() * 2;
    
    m_ratingPoints = 0;
    QString histogram = data.value("histogram").toString();
    QStringList histo = histogram.mid(1,histogram.size()-2).split(", ");
    for(int i=0; i<histo.size(); ++i) {
        int points = histo[i].toInt();
        m_ratingPoints = i*points;
    }
}

Rating::~Rating()
{
}

QString Rating::packageName() const
{
    return m_packageName;
}

QString Rating::applicationName() const
{
    return m_appName;
}

quint64 Rating::ratingCount() const
{
    return m_ratingCount;
}

int Rating::rating() const
{
    return m_rating;
}

int Rating::ratingPoints() const
{
    return m_ratingPoints;
}
