/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef STANDARDBACKENDUPDATER_H
#define STANDARDBACKENDUPDATER_H

#include "libmuonprivate_export.h"
#include <resources/AbstractBackendUpdater.h>
#include <QSet>

class AbstractResourcesBackend;

class MUONPRIVATE_EXPORT StandardBackendUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    public:
        explicit StandardBackendUpdater(AbstractResourcesBackend* parent = 0);

        virtual bool hasUpdates() const;
        virtual qreal progress() const;
        virtual void start();
        virtual long unsigned int remainingTime() const;
        
        virtual QList< AbstractResource* > toUpdate() const;
        virtual void addResources(QList< AbstractResource* > apps);
        virtual void removeResources(QList< AbstractResource* > apps);
        virtual void prepare();
        virtual void cleanup();
        virtual bool isAllMarked() const;
        virtual QDateTime lastUpdate() const;

    private:
        QSet<AbstractResource*> m_toUpgrade;
        AbstractResourcesBackend* m_backend;
        int m_preparedSize;
};

#endif // STANDARDBACKENDUPDATER_H

