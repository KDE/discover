/***************************************************************************
 *   Copyright (C) 2009 by Dominik Kapusta       <d@ayoy.net>              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of      *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to                     *
 *   the Free Software Foundation, Inc.,                                   *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


/*!
  \file interface.h

  This file is a part of libqoauth. You should not include it directly in your
  application. Instead please use <tt>\#include &lt;QtOAuth&gt;</tt>.
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>

#define slots Q_SLOTS
#define signals Q_SIGNALS

#include <QtCrypto>

#include "qoauth_global.h"
#include "qoauth_namespace.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace QOAuth {

class InterfacePrivate;

class QOAUTH_EXPORT Interface : public QObject
{
    Q_OBJECT

    Q_PROPERTY( QByteArray consumerKey READ consumerKey WRITE setConsumerKey )
    Q_PROPERTY( QByteArray consumerSecret READ consumerSecret WRITE setConsumerSecret )
    Q_PROPERTY( uint requestTimeout READ requestTimeout WRITE setRequestTimeout )
    Q_PROPERTY( bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors )
    Q_PROPERTY( int error READ error )

public:
    Interface( QObject *parent = 0 );
    Interface( QNetworkAccessManager *manager, QObject *parent = 0 );
    virtual ~Interface();

    QNetworkAccessManager* networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *manager);

    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(bool enabled);

    QByteArray consumerKey() const;
    void setConsumerKey( const QByteArray &consumerKey );

    QByteArray consumerSecret() const;
    void setConsumerSecret( const QByteArray &consumerSecret );

    uint requestTimeout() const;
    void setRequestTimeout( uint msec );

    int error() const;

//     bool setRSAPrivateKey( const QString &key,
//                            const QCA::SecureArray &passphrase = QCA::SecureArray() );
//     bool setRSAPrivateKeyFromFile( const QString &filename,
//                                    const QCA::SecureArray &passphrase = QCA::SecureArray() );


    ParamMap requestToken( const QString &requestUrl, HttpMethod httpMethod,
                           SignatureMethod signatureMethod = HMAC_SHA1, const ParamMap &params = ParamMap() );

    ParamMap accessToken( const QString &requestUrl, HttpMethod httpMethod, const QByteArray &token,
                          const QByteArray &tokenSecret, SignatureMethod signatureMethod = HMAC_SHA1,
                          const ParamMap &params = ParamMap() );

    QByteArray createParametersString( const QString &requestUrl, HttpMethod httpMethod,
                                       const QByteArray &token, const QByteArray &tokenSecret,
                                       SignatureMethod signatureMethod, const ParamMap &params, ParsingMode mode );

    QByteArray inlineParameters( const ParamMap &params, ParsingMode mode = ParseForRequestContent );


protected:
    InterfacePrivate * const d_ptr;

private:
    Q_DISABLE_COPY(Interface)
    Q_DECLARE_PRIVATE(Interface)
    Q_PRIVATE_SLOT(d_func(), void _q_parseReply(QNetworkReply *reply))
    Q_PRIVATE_SLOT(d_func(), void _q_setPassphrase(int id, const QCA::Event &event))
    Q_PRIVATE_SLOT(d_func(), void _q_handleSslErrors( QNetworkReply *reply,
                                                      const QList<QSslError> &errors ))

#ifdef UNIT_TEST
    friend class Ut_Interface;
    friend class Ft_Interface;
#endif
};

} // namespace QOAuth

#endif // INTERFACE_H
