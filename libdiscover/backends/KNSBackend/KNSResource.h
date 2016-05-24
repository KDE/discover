/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef KNSRESOURCE_H
#define KNSRESOURCE_H

#include <resources/AbstractResource.h>
#include <kns3/entry.h>
#include <attica/content.h>

#include "discovercommon_export.h"

class KNSBackend;
class DISCOVERCOMMON_EXPORT KNSResource : public AbstractResource
{
Q_OBJECT
public:
    explicit KNSResource(const KNS3::Entry& c, QString category, KNSBackend* parent);
    ~KNSResource() override;

    AbstractResource::State state() override;
    QString icon() const override;
    QString comment() override;
    QString name() override;
    QString packageName() const override;
    QStringList categories() override;
    QUrl homepage() override;
    QUrl thumbnailUrl() override;
    QUrl screenshotUrl() override;
    QString license() override;
    QString longDescription() override;
    QList<PackageState> addonsInformation() override { return QList<PackageState>(); }
    QString availableVersion() const override;
    QString installedVersion() const override;
    QString origin() const override;
    QString section() override;
    void fetchScreenshots() override;
    int size() override;
    void fetchChangelog() override;
    QStringList extends() const override;

    void setEntry(const KNS3::Entry& entry);
    KNS3::Entry entry() const;

private:
    const QString m_category;
    KNS3::Entry m_entry;
};

#endif // KNSRESOURCE_H
