/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FwupdResource.h"

#include <QDesktopServices>
#include <QStringList>
#include <QTimer>
#include <Transaction/AddonList.h>

FwupdResource::FwupdResource(FwupdDevice *device, AbstractResourcesBackend *parent)
    : FwupdResource(device,
                    QStringLiteral("org.fwupd.%1.device").arg(QString::fromUtf8(fwupd_device_get_id(device)).replace(QLatin1Char('/'), QLatin1Char('_'))),
                    parent)
{
}

FwupdResource::FwupdResource(FwupdDevice *device, const QString &id, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_id(id)
    , m_name(QString::fromUtf8(fwupd_device_get_name(device)))
    , m_deviceID(QString::fromUtf8(fwupd_device_get_id(device)))
{
    Q_ASSERT(!m_name.isEmpty());
    setObjectName(m_name);
    setDeviceDetails(device);
}

QString FwupdResource::availableVersion() const
{
    return m_availableVersion;
}

QStringList FwupdResource::categories()
{
    return m_categories;
}

QString FwupdResource::comment()
{
    return m_summary;
}

quint64 FwupdResource::size()
{
    return m_size;
}

QUrl FwupdResource::homepage()
{
    return m_homepage;
}

QUrl FwupdResource::helpURL()
{
    return {};
}

QUrl FwupdResource::bugURL()
{
    return {};
}

QUrl FwupdResource::donationURL()
{
    return {};
}

QVariant FwupdResource::icon() const
{
    return m_iconName;
}

QString FwupdResource::installedVersion() const
{
    return m_installedVersion;
}

QJsonArray FwupdResource::licenses()
{
    return {QJsonObject{{QStringLiteral("name"), m_license}}};
}

QString FwupdResource::longDescription()
{
    return m_description;
}

QString FwupdResource::name() const
{
    return m_displayName.isEmpty() ? m_name : m_displayName;
}

QString FwupdResource::vendor() const
{
    return m_vendor;
}

QString FwupdResource::origin() const
{
    return m_origin;
}

QString FwupdResource::packageName() const
{
    return m_name;
}

QString FwupdResource::section()
{
    return QStringLiteral("Firmware Updates");
}

AbstractResource::State FwupdResource::state()
{
    return m_state;
}

void FwupdResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    Q_EMIT changelogFetched(log);
}

void FwupdResource::setState(AbstractResource::State state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void FwupdResource::invokeApplication() const
{
    qWarning() << "Not Launchable";
}

QUrl FwupdResource::url() const
{
    return m_homepage;
}

QString FwupdResource::executeLabel() const
{
    return QStringLiteral("Not Invokable");
}

void FwupdResource::setReleaseDetails(FwupdRelease *release)
{
    m_origin = QString::fromUtf8(fwupd_release_get_remote_id(release));
    m_summary = QString::fromUtf8(fwupd_release_get_summary(release));
    m_vendor = QString::fromUtf8(fwupd_release_get_vendor(release));
    m_size = fwupd_release_get_size(release);
    m_availableVersion = QString::fromUtf8(fwupd_release_get_version(release));
    m_description = QString::fromUtf8((fwupd_release_get_description(release)));
    m_homepage = QUrl(QString::fromUtf8(fwupd_release_get_homepage(release)));
    m_license = QString::fromUtf8(fwupd_release_get_license(release));
    m_updateURI = QString::fromUtf8(fwupd_release_get_uri(release));
}

void FwupdResource::setDeviceDetails(FwupdDevice *dev)
{
    m_isLiveUpdatable = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_UPDATABLE);
    m_isOnlyOffline = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_ONLY_OFFLINE);
    m_needsReboot = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_NEEDS_REBOOT);

    if (fwupd_device_get_name(dev)) {
        QString vendorDesc = QString::fromUtf8(fwupd_device_get_name(dev));
        const QString vendorName = QString::fromUtf8(fwupd_device_get_vendor(dev));

        if (!vendorDesc.startsWith(vendorName))
            vendorDesc = vendorName + QLatin1Char(' ') + vendorDesc;
        m_displayName = vendorDesc;
    }
    m_summary = QString::fromUtf8(fwupd_device_get_summary(dev));
    m_vendor = QString::fromUtf8(fwupd_device_get_vendor(dev));
    m_releaseDate = QDateTime::fromSecsSinceEpoch(fwupd_device_get_created(dev)).date();
    m_availableVersion = QString::fromUtf8(fwupd_device_get_version(dev));
    m_description = QString::fromUtf8((fwupd_device_get_description(dev)));

    if (fwupd_device_get_icons(dev)->len >= 1)
        m_iconName = QString::fromUtf8((const gchar *)g_ptr_array_index(fwupd_device_get_icons(dev), 0)); // Check whether given icon exists or not!
    else
        m_iconName = QStringLiteral("device-notifier");
}

QString FwupdResource::cacheFile() const
{
    const auto filename_cache = FwupdBackend::cacheFile(QStringLiteral("fwupd"), QFileInfo(QUrl(m_updateURI).path()).fileName());
    Q_ASSERT(!filename_cache.isEmpty());
    return filename_cache;
}
