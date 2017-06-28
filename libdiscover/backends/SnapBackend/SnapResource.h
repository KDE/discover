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

#ifndef SNAPRESOURCE_H
#define SNAPRESOURCE_H

#include <resources/AbstractResource.h>
#include <QJsonObject>
#include <Snapd/Snap>

class SnapBackend;

class SnapResource : public AbstractResource
{
Q_OBJECT
public:
    explicit SnapResource(QSnapdSnap* snap, AbstractResource::State state, SnapBackend* parent);

    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QString license() override;
    int size() override;
    QUrl screenshotUrl() override;
    QUrl thumbnailUrl() override;
    QUrl homepage() override;
    QStringList categories() override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() override;
    QString packageName() const override;
    bool isTechnical() const override;
    bool canExecute() const override { return true; }
    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QList<PackageState> addonsInformation() override { return {}; }

    void setState(AbstractResource::State state);

public:
    AbstractResource::State m_state;

    QSnapdSnap* m_snap;
};

#endif // SNAPRESOURCE_H
