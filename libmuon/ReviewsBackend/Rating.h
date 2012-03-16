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

#ifndef RATING_H
#define RATING_H

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "libmuonprivate_export.h"

class MUONPRIVATE_EXPORT Rating : public QObject
{
Q_OBJECT
public:
    explicit Rating(const QVariantMap &data);
    ~Rating();

    QString packageName() const;
    QString applicationName() const;
    Q_SCRIPTABLE quint64 ratingCount() const;
    // 0.0 - 5.0 ranged rating multiplied by two and rounded for KRating*
    Q_SCRIPTABLE int rating() const;
    int ratingPoints() const;

private:
    QString m_packageName;
    QString m_appName;
    quint64 m_ratingCount;
    int m_rating;
    int m_ratingPoints;
};

#endif
