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

#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include <QAbstractListModel>
#include <QSet>
#include "libmuonprivate_export.h"

class AbstractSourcesBackend;
class MUONPRIVATE_EXPORT SourcesModel : public QAbstractListModel
{
    Q_OBJECT
    public:
        enum Roles {
            SourceBackend = Qt::UserRole+1
        };
        SourcesModel(QObject* parent = 0);
        ~SourcesModel();

        static SourcesModel* global();

        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent) const;
        void addSourcesBackend(AbstractSourcesBackend* sources);
        virtual QHash<int, QByteArray> roleNames() const;

    private:
        QList<AbstractSourcesBackend*> m_sources;
};

#endif // SOURCESMODEL_H
