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

#include <FlatpakBackend.h>

extern "C" {
#include <libappstream-glib/appstream-glib.h>
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

class AddonList;
class FlatpakResource : public AbstractResource
{
Q_OBJECT
public:
    explicit FlatpakResource(AsApp *app, FlatpakBackend *parent);

    AsApp *appstreamApp() const;
    QList<PackageState> addonsInformation() override;
    QString availableVersion() const override;
    QString appstreamId() const override;
    QString arch() const;
    bool canExecute() const override;
    QStringList categories() override;
    QString comment() override;
    QString commit() const;
    QStringList executables() const override;
    QVariant icon() const override;
    QString installedVersion() const override;
    bool isTechnical() const override;
    QUrl homepage() override;
    QString flatpakName() const;
    QString license() override;
    QString longDescription() override;
    QString name() override;
    QString origin() const override;
    QString packageName() const override;
    QUrl screenshotUrl() override;
    QString section() override;
    int size() override;
    AbstractResource::State state() override;
    QUrl thumbnailUrl() override;
    QString uniqueId() const;

    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;

    void setArch(const QString &arch);
    void setCommit(const QString &commit);
    void setFlatpakName(const QString &name);
    void setState(State state);
    void setSize(int size);
//     void setAddons(const AddonList& addons);
//     void setAddonInstalled(const QString& addon, bool installed);

public:
    QList<PackageState> m_addons;
    AsApp *m_app;
    QString m_arch;
    QString m_commit;
    QString m_flatpakName;
    int m_size;

};

#endif // FLATPAKRESOURCE_H
