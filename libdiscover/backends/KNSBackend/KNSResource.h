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
#include <KNSCore/EntryInternal>
#include <attica/content.h>

#include "discovercommon_export.h"

class KNSBackend;
class DISCOVERCOMMON_EXPORT KNSResource : public AbstractResource
{
Q_OBJECT
public:
    explicit KNSResource(const KNSCore::EntryInternal & c, QStringList categories, KNSBackend* parent);
    ~KNSResource() override;

    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() override;
    QString packageName() const override;
    QStringList categories() override;
    QUrl homepage() override;
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

    KNSBackend* knsBackend() const;

    void setEntry(const KNSCore::EntryInternal& entry);
    KNSCore::EntryInternal entry() const;

    bool canExecute() const override { return !executables().isEmpty(); }
    QStringList executables() const;
    void invokeApplication() const override;

    QUrl url() const override;

private:
    const QStringList m_categories;
    KNSCore::EntryInternal m_entry;
    KNS3::Entry::Status m_lastStatus;
};

#endif // KNSRESOURCE_H
