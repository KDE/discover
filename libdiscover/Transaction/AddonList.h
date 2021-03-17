/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ADDONLIST_H
#define ADDONLIST_H

#include <QStringList>
#include <QVector>

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT AddonList
{
public:
    enum State {
        None,
        ToInstall,
        ToRemove,
    };
    AddonList();
    AddonList(const AddonList &other) = default;

    bool isEmpty() const;
    QStringList addonsToInstall() const;
    QStringList addonsToRemove() const;
    State addonState(const QString& addonName) const;

    void addAddon(const QString &addon, bool toInstall);
    void resetAddon(const QString &addon);
    void clear();

private:
    QStringList m_toInstall;
    QStringList m_toRemove;
};

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const AddonList& addons);

#endif // ADDONLIST_H
