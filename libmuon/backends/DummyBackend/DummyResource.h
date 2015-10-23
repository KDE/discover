/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef DUMMYRESOURCE_H
#define DUMMYRESOURCE_H

#include <resources/AbstractResource.h>

class AddonList;
class DummyResource : public AbstractResource
{
Q_OBJECT
public:
    explicit DummyResource(QString  name, bool isTechnical, AbstractResourcesBackend* parent);

    virtual QList<PackageState> addonsInformation() override;
    virtual QString section() override;
    virtual QString origin() const override;
    virtual QString longDescription() override;
    virtual QString availableVersion() const override;
    virtual QString installedVersion() const override;
    virtual QString license() override;
    virtual int size() override;
    virtual QUrl screenshotUrl() override;
    virtual QUrl thumbnailUrl() override;
    virtual QUrl homepage() override;
    virtual QStringList categories() override;
    virtual AbstractResource::State state() override;
    virtual QString icon() const override;
    virtual QString comment() override;
    virtual QString name() override;
    virtual QString packageName() const override;
    virtual bool isTechnical() const override { return m_isTechnical; }
    virtual bool canExecute() const override { return true; }
    virtual void invokeApplication() const override;
    virtual void fetchChangelog() override;
    void setState(State state);
    void setAddons(const AddonList& addons);

    void setAddonInstalled(const QString& addon, bool installed);

public slots:
    void enableStateChanges();

public:
    QString m_name;
    AbstractResource::State m_state;
    QUrl m_screenshot;
    QUrl m_screenshotThumbnail;
    QString m_iconName;
    QList<PackageState> m_addons;
    bool m_isTechnical;
};

#endif // DUMMYRESOURCE_H
