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

#ifndef ORIGINSBACKEND_H
#define ORIGINSBACKEND_H

#include <QObject>
#include <QStringList>

class OriginsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList labels READ labels NOTIFY originsChanged);
    public:
        explicit OriginsBackend(QObject* parent = 0);
        QStringList labels() const;
        Q_SCRIPTABLE QString labelsOrigin(const QString& label) const;

    public slots:
        void addRepository(const QString& repository);
        void removeRepository(const QString& repository);

    private slots:
        void additionDone(int processErrorCode);
        void removalDone(int processErrorCode);

    signals:
        void originsChanged();
};

#endif // ORIGINSBACKEND_H
