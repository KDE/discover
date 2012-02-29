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

#ifndef ABSTRACTLOGINBACKEND_H
#define ABSTRACTLOGINBACKEND_H

#include <QObject>

class AbstractLoginBackend : public QObject
{
    Q_OBJECT
    public:
        AbstractLoginBackend(QObject* parent=0);
        virtual bool hasCredentials() const = 0;
        virtual QString displayName() const = 0;

    public slots:
        virtual void login() = 0;
        virtual void registerAndLogin() = 0;
        virtual void logout() = 0;
        virtual QByteArray token() const = 0;
        virtual QByteArray tokenSecret() const = 0;
        virtual QByteArray consumerKey() const = 0;
        virtual QByteArray consumerSecret() const = 0;

    signals:
        void connectionStateChanged();
};

#endif // ABSTRACTLOGINBACKEND_H
