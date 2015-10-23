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
#include <PackageKit/Details>

class PackageKitBackend;

class PackageKitResource : public AbstractResource
{
    Q_OBJECT
    public:
        explicit PackageKitResource(QString  packageName, QString  summary, PackageKitBackend* parent);
        virtual QString packageName() const override;
        virtual QString name() override;
        virtual QString comment() override;
        virtual QString longDescription() override;
        virtual QUrl homepage() override;
        virtual QString icon() const override;
        virtual QStringList categories() override;
        virtual QString license() override;
        virtual QString origin() const override;
        virtual QString section() override;
        virtual bool isTechnical() const override;
        virtual int size() override;
        virtual void fetchChangelog() override;
        
        virtual QList<PackageState> addonsInformation() override;
        virtual State state() override;
        
        virtual QUrl screenshotUrl() override;
        virtual QUrl thumbnailUrl() override;
        
        virtual QString installedVersion() const override;
        virtual QString availableVersion() const override;
        virtual QStringList allPackageNames() const;
        QString installedPackageId() const;
        QString availablePackageId() const;

        QMap<PackageKit::Transaction::Info, QStringList> packages() const { return m_packages; }

    public slots:
        void addPackageId(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void setDetails(const PackageKit::Details& details);
        void resetPackageIds();

    private slots:
        void updateDetail(const QString &packageID,
                          const QStringList &updates,
                          const QStringList &obsoletes,
                          const QStringList &vendorUrls,
                          const QStringList &bugzillaUrls,
                          const QStringList &cveUrls,
                          PackageKit::Transaction::Restart restart,
                          const QString &updateText,
                          const QString &changelog,
                          PackageKit::Transaction::UpdateState state,
                          const QDateTime &issued,
                          const QDateTime &updated);

    private:
        /** fetches details individually, it's better if done in batch, like for updates */
        void fetchDetails();
        PackageKitBackend* backend() const;

        QMap<PackageKit::Transaction::Info, QStringList> m_packages;
        QString m_summary;
        QString m_name;
        PackageKit::Details m_details;
};

#endif // PACKAGEKITRESOURCE_H
