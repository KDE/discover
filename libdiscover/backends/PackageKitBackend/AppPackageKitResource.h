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

#ifndef APPPACKAGEKITRESOURCE_H
#define APPPACKAGEKITRESOURCE_H

#include "PackageKitResource.h"
#include "PackageKitBackend.h"

class AppPackageKitResource : public PackageKitResource
{
    Q_OBJECT
    public:
        explicit AppPackageKitResource(const Appstream::Component& data, PackageKitBackend* parent);

        virtual bool isTechnical() const override;
        virtual QString name() override;
        virtual QString icon() const override;
        virtual QStringList mimetypes() const override;
        virtual QStringList categories() override;
        virtual QString longDescription() override;
        virtual QUrl homepage() override;
        virtual bool canExecute() const override;
        virtual QStringList executables() const override;
        virtual void invokeApplication() const override;
        virtual QString comment() override;
        virtual QString license() override;
        virtual QUrl screenshotUrl() override;
        virtual QUrl thumbnailUrl() override;
        virtual QStringList allPackageNames() const override;

    private:
        QStringList findProvides(Appstream::Provides::Kind kind) const;

        const Appstream::Component m_appdata;
};

#endif // APPPACKAGEKITRESOURCE_H
