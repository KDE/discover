/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef PACKAGEKITSOURCESBACKEND_H
#define PACKAGEKITSOURCESBACKEND_H

#include <PackageKit/Transaction>
#include <QStandardItemModel>
#include <resources/AbstractSourcesBackend.h>

class PackageKitSourcesBackend : public AbstractSourcesBackend
{
    Q_OBJECT
    public:
        PackageKitSourcesBackend(QObject* parent);

        QString name() const;
        QString idDescription();

        bool addSource(const QString& id);
        bool removeSource(const QString& id);

        QAbstractItemModel* sources();
        QList<QAction*> actions() const;


    private:
        void resetSources();
        void addRepositoryDetails(const QString &id, const QString &description, bool enabled);
        QStandardItem* findItemForId(const QString &id) const;
        void transactionError(PackageKit::Transaction::Error, const QString& message);

        QStandardItemModel* m_sources;
};

#endif // PACKAGEKITSOURCESBACKEND_H
