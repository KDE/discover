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

#ifndef UBUNTULOGINBACKEND_H
#define UBUNTULOGINBACKEND_H

#include <ReviewsBackend/AbstractLoginBackend.h>
#include <QVariant>
#include "LoginMetaTypes.h"

class ComUbuntuSsoApplicationCredentialsInterface;
class UbuntuLoginBackend : public AbstractLoginBackend
{
    Q_OBJECT
    public:
        UbuntuLoginBackend(QObject* parent=0);
        
        virtual void login();
        virtual void registerAndLogin();
        virtual void logout();
        virtual QString displayName() const;
        virtual bool hasCredentials() const;
        
        virtual QByteArray token() const;
        virtual QByteArray tokenSecret() const;
        virtual QByteArray consumerKey() const;
        virtual QByteArray consumerSecret() const;

    private slots:
        void credentialsError(const QString& app, const QString& a, const QString& b );
        void authorizationDenied(const QString& app);
        void successfulLogin(const QString& app, const MapString& credentials);

    private:
        QString appname() const;
        ComUbuntuSsoApplicationCredentialsInterface* m_interface;
        MapString m_credentials;
};

#endif // UBUNTULOGINBACKEND_H
