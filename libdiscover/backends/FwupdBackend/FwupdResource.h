/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "FwupdBackend.h"

#include <KLocalizedString>
#include <resources/AbstractResource.h>

class FwupdResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit FwupdResource(FwupdDevice *device, AbstractResourcesBackend *parent);
    explicit FwupdResource(FwupdDevice *device, const QString &id, AbstractResourcesBackend *parent);

    QList<PackageState> addonsInformation() override
    {
        return {};
    }
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
    QString vendor() const;
    AbstractResource::Type type() const override
    {
        return Technical;
    }
    bool canExecute() const override
    {
        return false;
    }
    void invokeApplication() const override;
    void fetchChangelog() override;
    QUrl url() const override;
    QString executeLabel() const override;
    QDate releaseDate() const override
    {
        return m_releaseDate;
    }
    QString sourceIcon() const override
    {
        return {};
    }
    QString author() const override
    {
        return {};
    }

    void setIsDeviceLocked(bool locked)
    {
        m_isDeviceLocked = locked;
    }
    void setDescription(const QString &description)
    {
        m_description = description;
    }
    void setInstalledVersion(const QString &version)
    {
        m_installedVersion = version;
    }

    void setState(AbstractResource::State state);
    void setReleaseDetails(FwupdRelease *release);

    QString id() const
    {
        return m_id;
    }

    QString deviceId() const
    {
        return m_deviceID;
    }

    QUrl updateURI() const
    {
        return QUrl(m_updateURI);
    }

    bool isDeviceLocked() const
    {
        return m_isDeviceLocked;
    }

    bool isOnlyOffline() const
    {
        return m_isOnlyOffline;
    }

    bool isLiveUpdatable() const
    {
        return m_isLiveUpdatable;
    }

    bool needsReboot() const
    {
        return m_needsReboot;
    }

    QString cacheFile() const;
    bool isRemovable() const override
    {
        return false;
    }

private:
    void setDeviceDetails(FwupdDevice *device);

    const QString m_id;
    const QString m_name;
    const QString m_deviceID;
    QString m_summary;
    QString m_description;
    QString m_installedVersion;
    QString m_availableVersion;
    QString m_vendor;
    QStringList m_categories;
    QString m_license;
    QString m_displayName;
    QDate m_releaseDate;

    AbstractResource::State m_state = None;
    QUrl m_homepage;
    QString m_iconName;
    quint64 m_size = 0;

    QString m_updateURI;
    bool m_isDeviceLocked = false; // True if device is locked!
    bool m_isOnlyOffline = false; // True if only offline updates
    bool m_isLiveUpdatable = false; // True if device is live updatable
    bool m_needsReboot = false; // True if device needs Reboot
    QString m_origin;
};
