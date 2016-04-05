/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtTest>
#include <QList>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <KCompressionDevice>
#include <ReviewsBackend/PopConParser.h>
#include <ReviewsBackend/Rating.h>

class PopConParserTest : public QObject
{
    Q_OBJECT
public:
    PopConParserTest() {}

private Q_SLOTS:
    void testPopConParser() {
        const QUrl ratingsUrl(QStringLiteral("http://appstream.kubuntu.co.uk/appstream-ubuntu-popcon-results.gz"));
        QNetworkAccessManager manager;
        auto reply = manager.get(QNetworkRequest(ratingsUrl));
        QSignalSpy spy(reply, &QNetworkReply::finished);
        QVERIFY(spy.count() || spy.wait());

        auto data = reply->readAll();
        QBuffer buffer(&data);

        QScopedPointer<QIODevice> dev(new KCompressionDevice(&buffer, false, KCompressionDevice::GZip));
        QVERIFY(dev->open(QIODevice::ReadOnly));
        auto m_ratings = PopConParser::parsePopcon(this, dev.data());
        for(auto it = m_ratings.constBegin(), itEnd = m_ratings.constEnd(); it!=itEnd; ++it) {
            QVERIFY(!it.key().isEmpty());
            Rating * r = it.value();
            QVERIFY(!r->packageName().isEmpty());
        }
        QVERIFY(!m_ratings.isEmpty());
        qDeleteAll(m_ratings);
    }
};

QTEST_MAIN( PopConParserTest )

#include "PopConParserTest.moc"
