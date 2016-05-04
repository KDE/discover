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

struct HackedComUbuntuSsoCredentialsManagementInterface;
class UbuntuLoginBackend : public AbstractLoginBackend
{
    Q_OBJECT
    public:
        explicit UbuntuLoginBackend(QObject* parent=nullptr);
        
        void login() override;
        void registerAndLogin() override;
        void logout() override;
        QString displayName() const override;
        bool hasCredentials() const override;
        
        QByteArray token() const override;
        QByteArray tokenSecret() const override;
        QByteArray consumerKey() const override;
        QByteArray consumerSecret() const override;

    private Q_SLOTS:
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
