/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

#ifndef LAUNCHLISTMODEL_H
#define LAUNCHLISTMODEL_H

#include "libmuonprivate_export.h"
#include <QStandardItemModel>

class Application;
class ApplicationBackend;
class MUONPRIVATE_EXPORT LaunchListModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(ApplicationBackend* backend READ backend WRITE setBackend)
    public:
        explicit LaunchListModel(QObject* parent = 0);
        void setBackend(ApplicationBackend* backend);
        ApplicationBackend* backend() const { return m_backend; }
        void invokeApplication(int row) const;

    private slots:
        void resetApplications();

    private:
        ApplicationBackend* m_backend;
};

#endif // LAUNCHLISTMODEL_H

