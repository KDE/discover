/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "AddonList.h"

enum {
    ToInstall = 0,
    ToRemove
};

AddonList::AddonList()
    : m_list(2)
{
}

AddonList::AddonList(const AddonList &other)
    : m_list(other.m_list)
{
}

bool AddonList::isEmpty() const
{
    return m_list.at(ToInstall).isEmpty() && m_list.at(ToRemove).isEmpty();
}

QStringList AddonList::addonsToInstall() const
{
    return m_list.at(ToInstall);
}

QStringList AddonList::addonsToRemove() const
{
    return m_list.at(ToRemove);
}

void AddonList::setAddonsToInstall(const QStringList &list)
{
    m_list[ToInstall] = list;
}

void AddonList::setAddonsToRemove(const QStringList &list)
{
    m_list[ToRemove] = list;
}

void AddonList::clear()
{
    m_list[ToInstall].clear();
    m_list[ToRemove].clear();
}
