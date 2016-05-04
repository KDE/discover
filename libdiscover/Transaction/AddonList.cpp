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

AddonList::AddonList()
{}

AddonList::AddonList(const AddonList &other) = default;

bool AddonList::isEmpty() const
{
    return m_toInstall.isEmpty() && m_toRemove.isEmpty();
}

QStringList AddonList::addonsToInstall() const
{
    return m_toInstall;
}

QStringList AddonList::addonsToRemove() const
{
    return m_toRemove;
}

void AddonList::addAddon(const QString &addon, bool toInstall)
{
    if (toInstall) {
        m_toInstall.append(addon);
        m_toRemove.removeAll(addon);
    } else {
        m_toInstall.removeAll(addon);
        m_toRemove.append(addon);
    }
}

void AddonList::resetAddon(const QString &addon)
{
    m_toInstall.removeAll(addon);
    m_toRemove.removeAll(addon);
}

void AddonList::clear()
{
    m_toInstall.clear();
    m_toRemove.clear();
}

AddonList::State AddonList::addonState(const QString& addonName) const
{
    if(m_toInstall.contains(addonName))
        return ToInstall;
    else if(m_toRemove.contains(addonName))
        return ToRemove;
    else
        return None;
}
