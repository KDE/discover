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

class AddonList;
class FlatpakBackend;
class FlatpakResource : public AbstractResource
{
Q_OBJECT
public:
    explicit FlatpakResource(AppStream::Component *component, FlatpakBackend *parent);

    enum ResourceType {
        DesktopApp = 0,
        Runtime
    };

    enum Scope {
        System = 0,
        User
    };

    Q_ENUM(Scope)

    static QString typeAsString(ResourceType type) {
        if (type == DesktopApp) {
            return QLatin1String("app");
        }
        return QLatin1String("runtime");
    }

    static QString scopeAsString(Scope scope) {
        if (scope == System) {
            return QLatin1String("system");
        }
        return QLatin1String("user");
    }

    AppStream::Component *appstreamComponent() const;
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
    QStringList executables() const override;
    QVariant icon() const override;
    QString installedVersion() const override;
    int installedSize() const;
    bool isTechnical() const override;
    QUrl homepage() override;
    QString flatpakName() const;
    QString license() override;
    QString longDescription() override;
    QString name() override;
    QString origin() const override;
    QString packageName() const override;
    QString runtime() const;
    QUrl screenshotUrl() override;
    Scope scope() const;
    QString scopeAsString() const;
    QString section() override;
    int size() override;
    AbstractResource::State state() override;
    QUrl thumbnailUrl() override;
    ResourceType type() const;
    QString typeAsString() const;
    QString uniqueId() const;

    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;

    void setArch(const QString &arch);
    void setBranch(const QString &branch);
    void setCommit(const QString &commit);
    void setDownloadSize(int size);
    void setIconPath(const QString &path);
    void setInstalledSize(int size);
    void setFlatpakName(const QString &name);
    void setOrigin(const QString &origin);
    void setRuntime(const QString &runtime);
    void setScope(Scope scope);
    void setState(State state);
    void setType(ResourceType type);
//     void setAddons(const AddonList& addons);
//     void setAddonInstalled(const QString& addon, bool installed);

    void updateFromRef(FlatpakRef* ref);

public:
    QList<PackageState> m_addons;
    AppStream::Component *m_appdata;
    FlatpakRefKind m_flatpakRefKind;
    QString m_arch;
    QString m_branch;
    QString m_commit;
    int m_downloadSize;
    QString m_flatpakName;
    QString m_iconPath;
    int m_installedSize;
    QString m_origin;
    QString m_runtime;
    Scope m_scope;
    AbstractResource::State m_state;
    ResourceType m_type;
};

#endif // FLATPAKRESOURCE_H
