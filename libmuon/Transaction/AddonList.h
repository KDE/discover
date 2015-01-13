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

#ifndef ADDONLIST_H
#define ADDONLIST_H

#include <QStringList>
#include <QtCore/QVector>

#include "libMuonCommon_export.h"

class MUONCOMMON_EXPORT AddonList
{
public:
    enum State {
        None,
        ToInstall,
        ToRemove
    };
    AddonList();
    AddonList(const AddonList &other);

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

#endif // ADDONLIST_H
