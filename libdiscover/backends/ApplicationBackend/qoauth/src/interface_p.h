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
  \file interface_p.h

  This file is a part of libqoauth and is considered strictly internal. You should not
  include it in your application. Instead please use <tt>\#include &lt;QtOAuth&gt;</tt>.
*/

#ifndef QOAUTH_P_H
#define QOAUTH_P_H

#include "interface.h"
#include <QPointer>
#include <QNetworkAccessManager>
#include <QtCrypto>

class QNetworkReply;
class QEventLoop;

namespace QOAuth {

class Interface;


class QOAUTH_EXPORT InterfacePrivate
{
    Q_DECLARE_PUBLIC(Interface)

public:
    enum Operation {
        RequestToken,
        Authorize,
        Authenticate,
        AccessToken
    };

    enum KeySource {
        KeyFromString,
        KeyFromFile
    };

    static const QByteArray OAuthVersion;
    static const QByteArray ParamToken;
    static const QByteArray ParamTokenSecret;

    static const QByteArray ParamConsumerKey;
    static const QByteArray ParamNonce;
    static const QByteArray ParamSignature;
    static const QByteArray ParamSignatureMethod;
    static const QByteArray ParamTimestamp;
    static const QByteArray ParamVersion;


    InterfacePrivate();
    void init();
    void setupNetworkAccessManager();

    QByteArray httpMethodToString( HttpMethod method );
    QByteArray signatureMethodToString( SignatureMethod method );
    ParamMap replyToMap( const QByteArray &data );
    QByteArray paramsToString( const ParamMap &parameters, ParsingMode mode );

    QByteArray createSignature( const QString &requestUrl, HttpMethod httpMethod,
                                SignatureMethod signatureMethod, const QByteArray &token,
                                const QByteArray &tokenSecret, ParamMap *params );

    // for PLAINTEXT only
    QByteArray createPlaintextSignature( const QByteArray &tokenSecret );

    ParamMap sendRequest( const QString &requestUrl, HttpMethod httpMethod, SignatureMethod signatureMethod,
                          const QByteArray &token, const QByteArray &tokenSecret, const ParamMap &params );

    // RSA-SHA1 stuff
    void setPrivateKey( const QString &source, const QCA::SecureArray &passphrase, KeySource from );
    void readKeyFromLoader( QCA::KeyLoader *keyLoader );

    bool privateKeySet;

    QCA::Initializer initializer;
    QCA::PrivateKey privateKey;
    QCA::SecureArray passphrase;
    QCA::EventHandler eventHandler;
    // end of RSA-SHA1 stuff

    bool ignoreSslErrors;
    QByteArray consumerKey;
    QByteArray consumerSecret;

    ParamMap replyParams;

    QPointer<QNetworkAccessManager> manager;
    QEventLoop *loop;

    uint requestTimeout;
    int error;

protected:
    Interface *q_ptr;

public:
    void _q_parseReply( QNetworkReply *reply );
    void _q_setPassphrase( int id, const QCA::Event &event );
    void _q_handleSslErrors( QNetworkReply *reply, const QList<QSslError> &errors );
};

} // namespace QOAuth

#endif // INTERFACE_P_H
