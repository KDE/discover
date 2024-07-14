/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ALPINEAPKRESOURCE_H
#define ALPINEAPKRESOURCE_H

#include <resources/AbstractResource.h>
#include <QtApkPackage.h>
#include <AppStreamQt/component.h>

class AddonList;

class AlpineApkResource : public AbstractResource
{
    Q_OBJECT

public:
    explicit AlpineApkResource(const QtApk::Package &apkPkg,
                               AppStream::Component &component,
                               AbstractResource::Type typ,
                               AbstractResourcesBackend *parent);

    QList<PackageState> addonsInformation() override;
    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QStringList categories() override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    AbstractResource::Type type() const override { return m_type; }
    bool canExecute() const override;
    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QString appstreamId() const override;
    QUrl url() const override;
    QString author() const override;
    QString sourceIcon() const override;
    QDate releaseDate() const override;

    void setState(State state);
    void setCategoryName(const QString &categoryName);
    void setOriginSource(const QString &originSource);
    void setSection(const QString &sectionName);
    void setAddons(const AddonList &addons);
    void setAddonInstalled(const QString &addon, bool installed);
    void setAvailableVersion(const QString &av);
    void setAppStreamData(const AppStream::Component &component);

private:
    bool hasAppStreamData() const;

public:
    AbstractResource::State m_state;
    const AbstractResource::Type m_type;
    QtApk::Package m_pkg;
    QString m_availableVersion;
    QString m_category;
    QString m_originSoruce;
    QString m_sectionName;
    QList<PackageState> m_addons;
    AppStream::Component m_appsC;
};

#endif // ALPINEAPKRESOURCE_H
