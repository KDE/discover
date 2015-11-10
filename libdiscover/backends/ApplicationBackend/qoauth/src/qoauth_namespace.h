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
  \file qoauth_namespace.h

  This file is a part of libqoauth. You should not include it directly in your
  application. Instead please use <tt>\#include &lt;QtOAuth&gt;</tt>.
*/

#ifndef QOAUTH_NAMESPACE_H
#define QOAUTH_NAMESPACE_H

#include <QMultiMap>
#include <QByteArray>

#include "qoauth_global.h"

/*!
  \namespace QOAuth
  \brief This namespace encapsulates all classes and definitions provided by libqoauth.
*/
namespace QOAuth {

    /*!
      \typedef ParamMap
      \brief A typedef for the data structure for storing request parameters
    */
    typedef QMultiMap<QByteArray,QByteArray> ParamMap;

    /*!
      \enum SignatureMethod
      \brief This enum type describes the signature method used by the request.

      There are 3 different signature methods defined by the
      <a href=http://oauth.net/core/1.0/#signing_process>OAuth protocol</a>. This enum
      is used to specify the method used by a specific request. Hence, one of its values
      must be passed as a parameter in any of the \ref QOAuth::Interface::requestToken(),
      \ref QOAuth::Interface::accessToken() or \ref QOAuth::Interface::createParametersString()
      method.
    */
    enum SignatureMethod {
        HMAC_SHA1, //!< Sets the signature method to HMAC-SHA1
        RSA_SHA1,  //!< Sets the signature method to RSA-SHA1 (not implemented yet)
        PLAINTEXT  //!< Sets the signature method to PLAINTEXT (not implemented yet)
    };

    /*!
      \enum HttpMethod
      \brief This enum type specifies the HTTP method used for creating
             a <a href=http://oauth.net/core/1.0/#anchor14>Signature Base String</a>
             and/or sending a request.

      The HTTP method has to be specified in QOAuth class for two reasons:
      \li to know what type of request should be prepared and sent
          (\ref QOAuth::Interface::requestToken() and \ref QOAuth::Interface::accessToken()),
      \li to prepare a correct signature, as the Signature Base String contains a parameter
          specifying the HTTP method used for request (\ref QOAuth::Interface::createParametersString()).

      \note For \ref QOAuth::Interface::requestToken() and \ref QOAuth::Interface::accessToken() methods
            only \ref GET and \ref POST methods are allowed.
    */
    enum HttpMethod {
        GET,   //!< Sets the HTTP method to GET
        POST,  //!< Sets the HTTP method to POST
        HEAD,  //!< Sets the HTTP method to HEAD
        PUT   //!< Sets the HTTP method to PUT
#ifndef Q_WS_WIN
        , DELETE //!< Sets the HTTP method to DELETE
#endif
    };

    /*!
      \enum ParsingMode
      \brief This enum type specifies the method of parsing parameters into
             a parameter string.

      When creating a parameters string for a custom request using
      \ref QOAuth::Interface::createParametersString() the parsing mode must be defined in order
      to prepare the string correctly.

      According to what is stated in <a href=http://oauth.net/core/1.0/#consumer_req_param>
      OAuth 1.0 Core specification</a>, parameters can be passed in a request to
      the Service Provider in 3 different ways. When using \ref QOAuth::Interface::createParametersString(),
      choose the one that suits you by setting \a ParsingMode appropriatelly.

      \sa QOAuth::Interface::createParametersString()
    */
    enum ParsingMode {
        ParseForRequestContent,     //!< Inline query format (<tt>foo=bar&bar=baz&baz=foo ...</tt>), suitable for POST requests
        ParseForInlineQuery,        /*!< Same as ParseForRequestContent, but prepends the string with a question mark -
                                     suitable for GET requests (appending parameters to the request URL) */
        ParseForHeaderArguments,    //!< HTTP request header format (parameters to be put inside a request header)
        ParseForSignatureBaseString //!< <a href=http://oauth.net/core/1.0/#anchor14>Signature Base String</a> format, meant for internal use.
    };

    /*!
      \enum ErrorCode
      \brief This enum type defines error types that are assigned to the
             \ref QOAuth::Interface::error property

      This error codes collection contains both network-related errors and those that
      can occur when incorrect arguments are provided to any of the class's methods.

      \sa QOAuth::Interface::error
    */
    enum ErrorCode {
        NoError = 200,              //!< No error occured (so far :-) )
        BadRequest = 400,           //!< Represents HTTP status code \c 400 (Bad Request)
        Unauthorized = 401,         //!< Represents HTTP status code \c 401 (Unauthorized)
        Forbidden = 403,            //!< Represents HTTP status code \c 403 (Forbidden)
        Timeout = 1001,             //!< Represents a request timeout error
        ConsumerKeyEmpty,           //!< Consumer key has not been provided
        ConsumerSecretEmpty,        //!< Consumer secret has not been provided
        UnsupportedHttpMethod,      /*!< The HTTP method is not supported by the request.
                                         \note \ref QOAuth::Interface::requestToken() and
                                         \ref QOAuth::Interface::accessToken()
                                         accept only HTTP GET and POST requests. */

        RSAPrivateKeyEmpty = 1101,  //!< RSA private key has not been provided
        //    RSAPassphraseError,         //!< RSA passphrase is incorrect (or has not been provided)
        RSADecodingError,           /*!< There was a problem decoding the RSA private key
                                     (the key is invalid or the provided passphrase is incorrect)*/
        RSAKeyFileError,            //!< The provided key file either doesn't exist or is unreadable.
        OtherError                  //!< A network-related error not specified above
    };


    /*!
      \brief Returns the supported OAuth protocol version
    */
    QOAUTH_EXPORT QByteArray supportedOAuthVersion();

    /*!
      \brief Returns the name of the Access Token argument parameter (<tt>oauth_token</tt> in
             current implementation)

      Useful when reading Service Provider's reply for \ref QOAuth::Interface::accessToken() request, e.g:
      \code
        QOAuth::Interface qoauth;
        QByteArray requestToken = "token";
        QByteArray requestTokenSecret = "secret";
        QOAuth::ParamMap reply = qoauth.accessToken( "http://example.com/access_token", QOAuth::POST,
                            token, tokenSecret, QOAuth::HMAC_SHA1 );

        if ( qoauth.error() == QOAuth::NoError ) {
            token = reply.value( QOAuth::tokenParameterName() );
            tokenSecret = reply.value( QOAuth::tokenSecretParameterName() );
        }
      \endcode
    */
    QOAUTH_EXPORT QByteArray tokenParameterName();

    /*!
      \brief Returns the name of the Token Secret argument parameter (<tt>oauth_token_secret</tt> in
             current implementation)
      \sa QOAuth::tokenParameterName()
    */
    QOAUTH_EXPORT QByteArray tokenSecretParameterName();

} // namespace QOAuth

#endif // QOAUTH_NAMESPACE_H
