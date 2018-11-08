/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#ifndef FLATPAKRESOURCE_H
#define FLATPAKRESOURCE_H

#include <resources/AbstractResource.h>

extern "C" {
#include <flatpak.h>
}

#include <AppStreamQt/component.h>

#include <QPixmap>

class AddonList;
class FlatpakBackend;
class FlatpakResource : public AbstractResource
{
Q_OBJECT
public:
    explicit FlatpakResource(const AppStream::Component &component, FlatpakInstallation* installation, FlatpakBackend *parent);

    enum PropertyKind {
        DownloadSize = 0,
        InstalledSize,
        RequiredRuntime
    };

    enum PropertyState {
        NotKnownYet = 0,
        AlreadyKnown,
        UnknownOrFailed,
    };

    enum ResourceType {
        DesktopApp = 0,
        Runtime,
        Extension,
        Source
    };

    struct Id {
        FlatpakInstallation * const installation;
        QString origin;
        FlatpakResource::ResourceType type;
        const QString id;
        QString branch;
        QString arch;
        bool operator!=(const Id& other) const { return !operator==(other); }
        bool operator==(const Id& other) const { return &other == this || (
               other.installation == installation
            && other.origin == origin
            && other.type == type
            && other.id == id
            && other.branch == branch
            && other.arch == arch
        ); }
    };

    static QString typeAsString(ResourceType type) {
        if (type == DesktopApp) {
            return QLatin1String("app");
        }
        return QLatin1String("runtime");
    }

    QString installationPath() const;
    static QString installationPath(FlatpakInstallation* installation);

    AppStream::Component appstreamComponent() const;
    QList<PackageState> addonsInformation() override;
    QString availableVersion() const override;
    QString appstreamId() const override;
    QString arch() const;
    QString branch() const;
    bool canExecute() const override;
    QStringList categories() override;
    QString comment() override;
    QString commit() const;
    int downloadSize() const;
    QVariant icon() const override;
    QString installedVersion() const override;
    int installedSize() const;
    AbstractResource::Type type() const override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QString flatpakFileType() const;
    QString flatpakName() const;
    QString license() override;
    QString longDescription() override;
    QString name() const override;
    QString origin() const override;
    QString packageName() const override;
    PropertyState propertyState(PropertyKind kind) const;
    QUrl resourceFile() const;
    QString runtime() const;
    QString section() override;
    int size() override;
    QString sizeDescription() override;
    AbstractResource::State state() override;
    ResourceType resourceType() const;
    QString typeAsString() const;
    FlatpakResource::Id uniqueId() const;
    QUrl url() const override;
    QDate releaseDate() const override;
    QString author() const override;
    QStringList extends() const override;

    FlatpakInstallation* installation() const { return m_id.installation; }

    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;

    void setBranch(const QString &branch);
    void setBundledIcon(const QPixmap &pixmap);
    void setDownloadSize(int size);
    void setIconPath(const QString &path);
    void setInstalledSize(int size);
    void setFlatpakFileType(const QString &fileType);
    void setFlatpakName(const QString &name);
    void setOrigin(const QString &origin);
    void setPropertyState(PropertyKind kind, PropertyState state);
    void setResourceFile(const QUrl &url);
    void setRuntime(const QString &runtime);
    void setState(State state);
    void setType(ResourceType type);
//     void setAddons(const AddonList& addons);
//     void setAddonInstalled(const QString& addon, bool installed);

    void updateFromRef(FlatpakRef* ref);
    QString ref() const;
    QString sourceIcon() const override;

Q_SIGNALS:
    void propertyStateChanged(PropertyKind kind, PropertyState state);

private:
    void setArch(const QString &arch);
    void setCommit(const QString &commit);

    const AppStream::Component m_appdata;
    FlatpakResource::Id m_id;
    FlatpakRefKind m_flatpakRefKind;
    QPixmap m_bundledIcon;
    QString m_commit;
    int m_downloadSize;
    QString m_flatpakFileType;
    QString m_flatpakName;
    QString m_iconPath;
    int m_installedSize;
    QHash<PropertyKind, PropertyState> m_propertyStates;
    QUrl m_resourceFile;
    QString m_runtime;
    AbstractResource::State m_state;
};

inline uint qHash(const FlatpakResource::Id &key)
{
    return qHash(key.installation)
         ^ qHash(key.origin)
         ^ qHash(key.type)
         ^ qHash(key.id)
         ^ qHash(key.branch)
         ^ qHash(key.arch)
         ;
}

#endif // FLATPAKRESOURCE_H
