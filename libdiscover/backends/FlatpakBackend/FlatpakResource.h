/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKRESOURCE_H
#define FLATPAKRESOURCE_H

#include <resources/AbstractResource.h>

#include "FlatpakPermission.h"
#include "flatpak-helper.h"

#include <AppStreamQt/component.h>

#include <QAbstractItemModel>
#include <QPixmap>

class AddonList;
class FlatpakBackend;
class FlatpakSource;

class FlatpakResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QStringList topObjects MEMBER s_objects CONSTANT)
    Q_PROPERTY(QStringList objects MEMBER s_bottomObjects CONSTANT)
    Q_PROPERTY(QString attentionText READ attentionText CONSTANT)
    Q_PROPERTY(QString dataLocation READ dataLocation CONSTANT)
    Q_PROPERTY(QString branch READ branch CONSTANT)
    Q_PROPERTY(bool hasDataButUninstalled READ hasDataButUninstalled NOTIFY hasDataButUninstalledChanged)
public:
    explicit FlatpakResource(const AppStream::Component &component, FlatpakInstallation *installation, FlatpakBackend *parent);

    enum PropertyKind {
        DownloadSize = 0,
        InstalledSize,
        RequiredRuntime,
    };
    Q_ENUM(PropertyKind)

    enum PropertyState {
        NotKnownYet = 0,
        AlreadyKnown,
        UnknownOrFailed,
        Fetching,
    };
    Q_ENUM(PropertyState)

    enum ResourceType {
        DesktopApp = 0,
        Runtime,
        Extension,
        Source,
    };
    Q_ENUM(ResourceType)

    enum FlatpakFileType {
        NotAFile,
        FileFlatpak,
        FileFlatpakRef,
    };
    Q_ENUM(FlatpakFileType)

    struct Id {
        const QString id;
        QString branch;
        QString arch;
        bool operator!=(const Id &other) const
        {
            return !operator==(other);
        }
        bool operator==(const Id &other) const
        {
            return &other == this
                || (other.id == id //
                    && other.branch == branch //
                    && other.arch == arch //
                );
        }
    };

    static QString typeAsString(ResourceType type)
    {
        if (type == DesktopApp) {
            return QLatin1String("app");
        }
        return QLatin1String("runtime");
    }

    QString installationPath() const;
    static QString installationPath(FlatpakInstallation *installation);

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
    quint64 downloadSize() const;
    QVariant icon() const override;
    QString installedVersion() const override;
    quint64 installedSize() const;
    AbstractResource::Type type() const override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    FlatpakFileType flatpakFileType() const;
    QString flatpakName() const;
    QJsonArray licenses() override;
    QString longDescription() override;
    QString name() const override;
    QString origin() const override;
    QString displayOrigin() const override;
    QString packageName() const override;
    PropertyState propertyState(PropertyKind kind) const;
    QUrl resourceFile() const;
    QString runtime() const;
    QString section() override;
    quint64 size() override;
    QString sizeDescription() override;
    AbstractResource::State state() override;
    ResourceType resourceType() const;
    QString typeAsString() const;
    FlatpakResource::Id uniqueId() const;
    QUrl url() const override;
    QDate releaseDate() const override;
    QString author() const override;
    QStringList extends() const override;
    QString versionString() override;

    FlatpakInstallation *installation() const
    {
        return m_installation;
    }

    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QSet<QString> alternativeAppstreamIds() const override;
    QStringList mimetypes() const override;

    void setBranch(const QString &branch);
    void setBundledIcon(const QPixmap &pixmap);
    void setDownloadSize(quint64 size);
    void setIconPath(const QString &path);
    void setInstalledSize(quint64 size);
    void setFlatpakFileType(FlatpakFileType fileType);
    void setFlatpakName(const QString &name);
    void setOrigin(const QString &origin);
    void setDisplayOrigin(const QString &displayOrigin);
    void setPropertyState(PropertyKind kind, PropertyState state);
    void setResourceFile(const QUrl &url);
    void setRuntime(const QString &runtime);
    void setState(State state, bool shouldEmit = true);
    void setType(ResourceType type);
    void setResourceLocation(const QUrl &location)
    {
        m_resourceLocation = location;
    }

    void updateFromRef(FlatpakRef *ref);
    QString ref() const;
    QString sourceIcon() const override;
    QString installPath() const;
    void updateFromAppStream();
    void setArch(const QString &arch);
    QString attentionText() const;
    QString dataLocation() const;
    bool hasDataButUninstalled() const;
    Q_INVOKABLE QAbstractListModel *permissionsModel();

    void setTemporarySource(const QSharedPointer<FlatpakSource> &temp)
    {
        m_temp = temp;
    }
    QSharedPointer<FlatpakSource> temporarySource() const
    {
        return m_temp;
    }

    Q_INVOKABLE void clearUserData();
    Q_INVOKABLE int versionCompare(FlatpakResource *resource) const;

    const AppStream::Component appdata() const
    {
        return m_appdata;
    }

    QString contentRatingText() const override;
    QString contentRatingDescription() const override;
    ContentIntensity contentRatingIntensity() const override;
    uint contentRatingMinimumAge() const override;

Q_SIGNALS:
    void hasDataButUninstalledChanged();
    void propertyStateChanged(FlatpakResource::PropertyKind kind, FlatpakResource::PropertyState state);

private:
    void setCommit(const QString &commit);
    void loadPermissions();

    const AppStream::Component m_appdata;
    FlatpakResource::Id m_id;
    FlatpakRefKind m_flatpakRefKind;
    QPixmap m_bundledIcon;
    QString m_commit;
    qint64 m_downloadSize;
    FlatpakFileType m_flatpakFileType = FlatpakResource::NotAFile;
    QString m_flatpakName;
    QString m_iconPath;
    qint64 m_installedSize;
    QHash<PropertyKind, PropertyState> m_propertyStates;
    QUrl m_resourceFile;
    QUrl m_resourceLocation;
    QString m_runtime;
    AbstractResource::State m_state;
    FlatpakInstallation *const m_installation;
    QString m_origin;
    QString m_displayOrigin;
    mutable QString m_availableVersion;
    FlatpakResource::ResourceType m_type = DesktopApp;
    QSharedPointer<FlatpakSource> m_temp;
    QVector<FlatpakPermission> m_permissions;
    static const QStringList s_objects;
    static const QStringList s_bottomObjects;
};

inline uint qHash(const FlatpakResource::Id &key)
{
    return qHash(key.id) ^ qHash(key.branch) ^ qHash(key.arch);
}

#endif // FLATPAKRESOURCE_H
