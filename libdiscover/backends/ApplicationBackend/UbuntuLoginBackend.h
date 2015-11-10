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

#ifndef UBUNTULOGINBACKEND_H
#define UBUNTULOGINBACKEND_H

#include <ReviewsBackend/AbstractLoginBackend.h>
#include <QVariant>

class HackedComUbuntuSsoCredentialsManagementInterface;
class UbuntuLoginBackend : public AbstractLoginBackend
{
    Q_OBJECT
    public:
        UbuntuLoginBackend(QObject* parent=0);
        
        void login();
        void registerAndLogin();
        void logout();
        QString displayName() const;
        bool hasCredentials() const;
        
        QByteArray token() const;
        QByteArray tokenSecret() const;
        QByteArray consumerKey() const;
        QByteArray consumerSecret() const;

    private slots:
        void credentialsError(const QString& app, const QMap<QString,QString>& a);
        void authorizationDenied(const QString& app);
        void successfulLogin(const QString& app, const QMap<QString,QString>& credentials);

    private:
        QString appname() const;
        QString winId() const;
        HackedComUbuntuSsoCredentialsManagementInterface* m_interface;
        QMap<QString,QString> m_credentials;
};

#endif // UBUNTULOGINBACKEND_H
