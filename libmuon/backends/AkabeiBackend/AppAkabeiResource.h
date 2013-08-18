/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef APPPACKAGEKITRESOURCE_H
#define APPPACKAGEKITRESOURCE_H

#include "AkabeiResource.h"
#include "AkabeiBackend.h"

class AppPackageKitResource : public AkabeiResource
{
    Q_OBJECT
    public:
        explicit AppPackageKitResource(const ApplicationData& data, Akabei::Package * pkg, AkabeiBackend* parent);
        virtual bool isTechnical() const;
        virtual QString name();
        virtual QString icon() const;
        virtual QString mimetypes() const;
        virtual QString categories();
        virtual QString longDescription() const;
        virtual QUrl homepage() const;
        virtual bool canExecute() const { return true; }
        virtual QStringList executables() const;
        virtual void invokeApplication() const;

    private:
        ApplicationData m_appdata;
};

#endif // APPPACKAGEKITRESOURCE_H
