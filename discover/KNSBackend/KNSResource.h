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

#include <libmuon/resources/AbstractResource.h>
#include <knewstuff3/entry.h>
#include <attica/content.h>

class KNSResource : public AbstractResource
{
Q_OBJECT
public:
    explicit KNSResource(const Attica::Content& c, const QString& category, const QString& icon, QObject* parent = 0);
    virtual ~KNSResource();

    void setStatus(KNS3::Entry::Status status);

    virtual AbstractResource::State state();
    virtual QString icon() const;
    virtual QString comment();
    virtual QString name();
    virtual QString packageName() const;
    virtual QString categories();
    virtual QUrl homepage() const;
    virtual QUrl thumbnailUrl();
    virtual QString license();
    virtual QString longDescription() const;
    virtual QList<PackageState> addonsInformation() { return QList<PackageState>(); }

    Attica::Content& content();
    void setEntry(const KNS3::Entry& entry);
    KNS3::Entry* entry() const;

private:
    KNS3::Entry::Status m_status;
    Attica::Content m_content;
    QString m_category;
    QString m_icon;
    KNS3::Entry* m_entry;
};

#endif // KNSRESOURCE_H
