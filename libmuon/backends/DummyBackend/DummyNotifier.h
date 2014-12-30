/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
#ifndef DUMMYNOTIFIER_H
#define DUMMYNOTIFIER_H

#include <BackendNotifierModule.h>

class DummyNotifier : public BackendNotifierModule
{
Q_OBJECT
Q_PLUGIN_METADATA(IID "org.kde.muon.BackendNotifierModule")
Q_INTERFACES(BackendNotifierModule)
public:
    DummyNotifier(QObject* parent = 0);
    virtual ~DummyNotifier();

    virtual bool isSystemUpToDate() const Q_DECL_OVERRIDE;
    virtual void recheckSystemUpdateNeeded() Q_DECL_OVERRIDE;
    virtual uint securityUpdatesCount() Q_DECL_OVERRIDE;
    virtual uint updatesCount() Q_DECL_OVERRIDE;
};

#endif
