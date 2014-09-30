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
#include <PackageKit/Transaction>

class PackageKitBackend;

class PackageKitResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QString license READ license NOTIFY licenseChanged)
    public:
        explicit PackageKitResource(const QString &packageId, PackageKit::Transaction::Info info, const QString &summary, PackageKitBackend* parent);
        virtual QString packageName() const;
        virtual QString name();
        virtual QString comment();
        virtual QString longDescription();
        virtual QUrl homepage();
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
        QString installedPackageId() const;
        QString availablePackageId() const;

    public slots:
        void addPackageId(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void setDetails(const PackageKit::Details& details);
        void resetPackageIds();

    signals:
        void licenseChanged();

    private:
        PackageKitBackend * m_backend;
        QString m_installedPackageId;
        QString m_availablePackageId;
        PackageKit::Transaction::Info m_info;
        QString m_summary;
        QString m_license;
        PackageKit::Transaction::Group m_group;
        QString m_detail;
        QString m_url;
        qulonglong m_size;
        QString m_name;
        QString m_icon;
        QString m_availableVersion;
        QString m_installedVersion;
};

#endif // PACKAGEKITRESOURCE_H
