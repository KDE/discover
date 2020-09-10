/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PACKAGEKITRESOURCE_H
#define PACKAGEKITRESOURCE_H

#include <resources/AbstractResource.h>
#include <PackageKit/Transaction>
#include <PackageKit/Details>

class PackageKitBackend;

class PackageKitResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QStringList objects MEMBER m_objects CONSTANT)
    public:
        explicit PackageKitResource(QString  packageName, QString  summary, PackageKitBackend* parent);
        QString packageName() const override;
        QString name() const override;
        QString comment() override;
        QString longDescription() override;
        QUrl homepage() override;
        QVariant icon() const override;
        QStringList categories() override;
        QJsonArray licenses() override;
        QString origin() const override;
        QString section() override;
        AbstractResource::Type type() const override;
        int size() override;
        void fetchChangelog() override;
        void fetchUpdateDetails() override;

        QList<PackageState> addonsInformation() override;
        State state() override;
        
        QString installedVersion() const override;
        QString availableVersion() const override;
        QString author() const override { return {}; }
        virtual QStringList allPackageNames() const;
        QString installedPackageId() const;
        QString availablePackageId() const;

        void clearPackageIds() { m_packages.clear(); }

        QMap<PackageKit::Transaction::Info, QStringList> packages() const { return m_packages; }

        PackageKitBackend* backend() const;

        static QString joinPackages(const QStringList& pkgids, const QString &_sep, const QString &shadowPackageName);

        void invokeApplication() const override {}
        bool canExecute() const override { return false; }

        QString sizeDescription() override;
        void setDependenciesCount(int count);

        QString sourceIcon() const override;

        QDate releaseDate() const override { return {}; }

        virtual QString changelog() const { return {}; }

        bool extendsItself() const;

    Q_SIGNALS:
        void dependenciesFound(const QJsonObject& dependencies);

    public Q_SLOTS:
        void addPackageId(PackageKit::Transaction::Info info, const QString &packageId, bool arch);
        void setDetails(const PackageKit::Details& details);

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

        void failedFetchingDetails(PackageKit::Transaction::Error, const QString& msg);

    protected:
        PackageKit::Details m_details;

    private:
        void fetchDependencies();
        /** fetches details individually, it's better if done in batch, like for updates */
        virtual void fetchDetails();

        QMap<PackageKit::Transaction::Info, QStringList> m_packages;
        const QString m_summary;
        const QString m_name;
        int m_dependenciesCount = -1;
        static const QStringList m_objects;
};

#endif // PACKAGEKITRESOURCE_H
