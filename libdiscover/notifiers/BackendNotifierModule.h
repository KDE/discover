/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef BACKENDNOTIFIERMODULE_H
#define BACKENDNOTIFIERMODULE_H

#include <QObject>
#include "discovernotifiers_export.h"

class DISCOVERNOTIFIERS_EXPORT BackendNotifierModule : public QObject
{
Q_OBJECT
public:
    BackendNotifierModule(QObject* parent = nullptr);
    virtual ~BackendNotifierModule();

    /*** @returns \c true if there are any updates, \c false if not **/
    virtual bool isSystemUpToDate() const = 0;

    /*** Check for new updates. Emits @see foundUpdates when it finds something. **/
    virtual void recheckSystemUpdateNeeded() = 0;

    /*** @returns count of !security updates only. **/
    virtual uint updatesCount() = 0;

    /*** @returns count of security updates only. **/
    virtual uint securityUpdatesCount() = 0;

Q_SIGNALS:
    /**
     * This signal is emitted when any new updates are available.
     * @see recheckSystemUpdateNeeded
     */
    void foundUpdates();
};

Q_DECLARE_INTERFACE(BackendNotifierModule, "org.kde.muon.BackendNotifierModule")

#endif
