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
#ifndef ABSTRACTKDEDMODULE_H
#define ABSTRACTKDEDMODULE_H

#include <KDEDModule>

#include "libmuonprivate_export.h"

class MUONPRIVATE_EXPORT AbstractKDEDModule : public KDEDModule
{
    Q_OBJECT
    Q_PROPERTY(bool systemUpToDate READ isSystemUpToDate WRITE setSystemUpToDate);
    Q_PROPERTY(UpdateType updateType READ updateType WRITE setUpdateType);
public:    
    enum UpdateType {
        NormalUpdate = 0,
        SecurityUpdate = 1
    };
    Q_ENUMS(UpdateType);
    virtual ~AbstractKDEDModule();

    bool isSystemUpToDate() const;
    UpdateType updateType() const;

public slots:
    virtual void configurationChanged() = 0;
    virtual void recheckSystemUpdateNeeded() = 0;

signals:
    void systemUpdateNeeded();

protected:
    AbstractKDEDModule(const QString &name, QObject * parent);
    
    void setSystemUpToDate(bool systemUpToDate);
    void setUpdateType(UpdateType updateType);
    
private:
    class Private;
    Private * d;
    Q_PRIVATE_SLOT(d, void __k__showMuon(bool, QPoint));
};

#endif //ABSTRACTKDEDMODULE_H
