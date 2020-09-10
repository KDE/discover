/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AddonList.h"
#include "libdiscover_debug.h"

AddonList::AddonList()
{}

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

QDebug operator<<(QDebug debug, const AddonList& addons)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AddonsList(";
    debug.nospace() << "install:" << addons.addonsToInstall() << ',';
    debug.nospace() << "remove:" << addons.addonsToRemove() << ',';
    debug.nospace() << ')';
    return debug;
}
