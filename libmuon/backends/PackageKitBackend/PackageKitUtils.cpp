/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PackageKitUtils.h"
#include <QStringList>

int PackageKitUtils::compare_versions(const QString & a, const QString & b)
{
    /* First split takes pkgrels */
    QStringList withpkgrel1 = a.split("-");
    QStringList withpkgrel2 = b.split("-");
    QString pkgrel1, pkgrel2;

    if (withpkgrel1.size() >= 2) {
        pkgrel1 = withpkgrel1.at(1);
    }
    if (withpkgrel2.size() >= 2) {
        pkgrel2 = withpkgrel2.at(1);
    }

    for (int i = 0; i != withpkgrel1.count(); i++) {
        QString s1( withpkgrel1.at(i) ); /* takes the rest */
        if (withpkgrel2.count() < i)
            return -1;
        QString s2( withpkgrel2.at(i) );

        /* Second split is to separate actual version numbers (or strings) */
        QStringList v1 = s1.split(".");
        QStringList v2 = s2.split(".");

        QStringList::iterator i1 = v1.begin();
        QStringList::iterator i2 = v2.begin();

        for (; i1 < v1.end() && i2 < v2.end() ; i1++, i2++) {
            if ((*i1).length() > (*i2).length())
                return 1;
            if ((*i1).length() < (*i2).length())
                return -1;
            int p1 = i1->toInt();
            int p2 = i2->toInt();

            if (p1 > p2) {
                return 1;
            } else if (p1 < p2) {
                return -1;
            }
        }

        /* This is, like, v1 = 2.3 and v2 = 2.3.1: v2 wins */
        if (i1 == v1.end() && i2 != v2.end()) {
            return -1;
        }

        /* The opposite case as before */
        if (i2 == v2.end() && i1 != v1.end()) {
            return 1;
        }

        /* The rule explained above */
        if ((!pkgrel1.isEmpty() && pkgrel2.isEmpty()) || (pkgrel1.isEmpty() && !pkgrel2.isEmpty())) {
            return 0;
        }

        /* Normal pkgrel comparison */
        int pg1 = pkgrel1.toInt();
        int pg2 = pkgrel2.toInt();

        if (pg1 > pg2) {
            return 1;
        } else if (pg2 > pg1) {
            return -1;
        }
    }

    return 0;
}

