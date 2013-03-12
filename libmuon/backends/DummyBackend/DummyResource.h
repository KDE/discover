/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef DUMMYRESOURCE_H
#define DUMMYRESOURCE_H

#include <resources/AbstractResource.h>

class DummyResource : public AbstractResource
{
Q_OBJECT
public:
    explicit DummyResource(const QString& name, AbstractResourcesBackend* parent);

    virtual QList<PackageState> addonsInformation();
    virtual QString section();
    virtual QString origin() const;
    virtual QString longDescription() const;
    virtual QString availableVersion() const;
    virtual QString installedVersion() const;
    virtual QString license();
    virtual int downloadSize();
    virtual QUrl screenshotUrl();
    virtual QUrl thumbnailUrl();
    virtual QUrl homepage() const;
    virtual QStringList categories();
    virtual AbstractResource::State state();
    virtual QString icon() const;
    virtual QString comment();
    virtual QString name();
    virtual QString packageName() const;
    virtual bool isTechnical() const { return false; }
    void setState(State state);
    virtual bool canExecute() const { return true; }
    virtual void invokeApplication() const;
    virtual void fetchChangelog();

public:
    QString m_name;
    AbstractResource::State m_state;
};

#endif // DUMMYRESOURCE_H
