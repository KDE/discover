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

#include "UbuntuLoginBackend.h"
#include <QDebug>
#include <QDBusMetaType>
#include <QApplication>
#include <QWidget>
#include <KLocalizedString>
#include "ubuntu_sso.h"
#include "LoginMetaTypes.h"

UbuntuLoginBackend::UbuntuLoginBackend(QObject* parent)
    : AbstractLoginBackend(parent)
{
    qDBusRegisterMetaType<MapString>();
    m_interface = new ComUbuntuSsoApplicationCredentialsInterface( "com.ubuntu.sso", "/credentials", QDBusConnection::sessionBus(), this);
    connect(m_interface, SIGNAL(CredentialsError(QString,QString,QString)), SLOT(credentialsError(QString,QString,QString)));
    connect(m_interface, SIGNAL(AuthorizationDenied(QString)), SLOT(authorizationDenied(QString)));
    connect(m_interface, SIGNAL(CredentialsFound(QString, MapString)), this, SLOT(successfulLogin(QString, MapString)));
    qDebug() << "falalala" << m_interface->lastError().message();
    
    QDBusPendingReply< QMap<QString,QString> > cred = m_interface->find_credentials(appname());
    cred.waitForFinished();
    qDebug() << "lalala" << cred.error().message();
    m_credentials = cred.value();
}

void UbuntuLoginBackend::login()
{
    QDBusPendingReply< void > ret = m_interface->login_to_get_credentials(appname(), i18n("log in to the Ubuntu SSO service"),
                                          qobject_cast<QApplication*>(qApp)->activeWindow()->winId());
}

void UbuntuLoginBackend::registerAndLogin()
{
    m_interface->login_or_register_to_get_credentials(appname(), QString(), i18n("log in to the Ubuntu SSO service"),
                                          qobject_cast<QApplication*>(qApp)->activeWindow()->winId());
}

QString UbuntuLoginBackend::displayName() const
{
    return m_credentials["name"];
}

bool UbuntuLoginBackend::hasCredentials() const
{
    return !m_credentials.isEmpty();
}

void UbuntuLoginBackend::successfulLogin(const QString& app, const QMap<QString,QString>& credentials)
{
    qDebug() << "logged in" << appname() << app << credentials;
    if(app==appname()) {
        m_credentials = credentials;
        emit connectionStateChanged();
    }
}

QString UbuntuLoginBackend::appname() const
{
    return QCoreApplication::instance()->applicationName();
}

void UbuntuLoginBackend::authorizationDenied(const QString& app)
{
    qDebug() << "denied" << app;
    if(app==appname())
        emit connectionStateChanged();
}

void UbuntuLoginBackend::credentialsError(const QString& app, const QString& a, const QString& b)
{
    //TODO: provide error message?
    qDebug() << "error" << app << a << b;
    if(app==appname())
        emit connectionStateChanged();
}

void UbuntuLoginBackend::logout()
{
    m_interface->clear_token(appname());
    m_credentials.clear();
    emit connectionStateChanged();
}
