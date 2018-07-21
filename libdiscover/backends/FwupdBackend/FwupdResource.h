/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#ifndef FWUPDRESOURCE_H
#define FWUPDRESOURCE_H

#include <resources/AbstractResource.h>
#include "FwupdBackend.h"
#include <KLocalizedString>

class AddonList;
class FwupdResource : public AbstractResource
{
Q_OBJECT
public:
    explicit FwupdResource(QString  name, bool isTechnical, AbstractResourcesBackend* parent);

    QList<PackageState> addonsInformation() override;
    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QString license() override;
    int size() override;
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
    bool isTechnical() const override { return m_isTechnical; }
    bool canExecute() const override { return false; }
    void invokeApplication() const override;
    void fetchChangelog() override;
    QUrl url() const override;
    QString executeLabel() const override;
    
    void setState(State state);
    void setSize(int size) { m_size = size; }
    void setAddons(const AddonList& addons);
    void setId(const QString &id){m_id = id;}
    void setName(const QString &name){ m_name = name;}
    void setSummary(const QString &summary){ m_summary = summary;}
    void setDescription(const QString &description){ m_description = description;}
    void setVersion(const QString &version){ m_version = version;}
    void setVendor(const QString &vendor){ m_vendor = vendor;}
    void addCategories(const QString &category);
    void setHomePage(const QUrl &homepage){  m_homepage = homepage;}
    void setLicense(const QString &license){ m_license = license;}
    void setIconName(const QString &iconName){ m_iconName = iconName;}
    virtual QStringList allResourceNames() const;
    
    void setIsDeviceLocked(bool status){ isDeviceLocked = status;}
    void setDeviceID(const QString &deviceID){ m_deviceID = deviceID;}
    void setUpdateURI(const QString &updateURI){m_updateURI = updateURI;}
    
    void setAddonInstalled(const QString& addon, bool installed);
    QString sourceIcon() const override { return QStringLiteral("player-time"); }
    QDate releaseDate() const override { return {}; }

public:
    QString m_id;
    QString m_name;
    QString m_summary;
    QString m_description;
    QString m_version;
    QString m_updateVersion;
    QString m_vendor;
    QStringList m_categories;
    QString m_license;
    
    AbstractResource::State m_state;
    QUrl m_homepage;
    QString m_iconName;
    QList<PackageState> m_addons;
    bool m_isTechnical;
    int m_size;
    
    QString m_deviceID;
    QString m_updateURI;
    QFile* m_file;
    bool isDeviceLocked = false; // True if device is locked!
    bool isOnlyOffline = false; // True if only offline updates
    bool isLiveUpdatable = false; // True if device is live updatable
    bool needsReboot = false; // True if device needs Reboot
    bool isDeviceRemoval = false; //True if device is Removal
    QString guidString;
    

    QList<FwupdResource*> m_releases; // A list of all refrences to releases of a device.
};

#endif // FWUPDRESOURCE_H
