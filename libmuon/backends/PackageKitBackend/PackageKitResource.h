/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef PACKAGEKITRESOURCE_H
#define PACKAGEKITRESOURCE_H

#include <resources/AbstractResource.h>
#include <PackageKit/packagekit-qt2/Package>

class PackageKitResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QString license READ license NOTIFY licenseChanged)
    public:
        explicit PackageKitResource(const PackageKit::Package& p, AbstractResourcesBackend* parent);
        virtual QString packageName() const;
        virtual QString name();
        virtual QString comment();
        virtual QString longDescription() const;
        virtual QUrl homepage() const;
        virtual QString icon() const;
        virtual QStringList categories();
        virtual QString license();
        virtual QString origin() const;
        virtual QString section();
        virtual bool isTechnical() const;
        virtual int downloadSize();
        virtual void fetchChangelog();
        
        virtual QList<PackageState> addonsInformation();
        virtual State state();
        
        virtual QUrl screenshotUrl();
        virtual QUrl thumbnailUrl();
        
        virtual QString installedVersion() const;
        virtual QString availableVersion() const;
        PackageKit::Package package() const;

    public slots:
        void updatePackage(const PackageKit::Package& p);

    signals:
        void licenseChanged();

    private:
        void fetchDetails();
        PackageKit::Package m_package;
};

#endif // PACKAGEKITRESOURCE_H
