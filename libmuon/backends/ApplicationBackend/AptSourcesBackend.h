/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef APTSOURCESBACKEND_H
#define APTSOURCESBACKEND_H

#include <QStandardItemModel>
#include <resources/AbstractSourcesBackend.h>
#include <LibQApt/SourcesList>

class ApplicationBackend;
class SourceItem;

class AptSourcesBackend : public AbstractSourcesBackend
{
Q_OBJECT
public:
    enum Roles {
        UriRole
    };
    
    AptSourcesBackend(ApplicationBackend* backend);
    virtual QAbstractItemModel* sources();
    virtual bool removeSource(const QString& uri);
    virtual bool addSource(const QString& uri);
    virtual QString idDescription();
    virtual QString name() const;
    ApplicationBackend* appsBackend() const;

private slots:
    void load();
    void removalDone(int processErrorCode);
    void additionDone(int processErrorCode);

private:
    SourceItem* sourceForUri(const QString& uri);

    QStandardItemModel* m_sources;
    QApt::SourcesList m_sourcesList;
};

#endif // APTSOURCESBACKEND_H
