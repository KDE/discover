/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AppstreamUtils.h"
#include <qxmlstream.h>
#include <QFile>
#include <QDebug>

namespace {

QStringList joinLists(const QList<QStringList>& list)
{
    QStringList ret;
    foreach(const QStringList& l, list) {
        ret += l;
    }
    return ret;
}

QHash<QString, QStringList> readElementList(QXmlStreamReader* reader, const QString& listedTagName)
{
    Q_UNUSED(listedTagName)
    Q_ASSERT(reader->isStartElement());
    QHash<QString, QStringList> ret;
    QStringRef startTag = reader->name();
    while(!(reader->isEndElement() && reader->name()==startTag)) {
        reader->readNext();

        if(reader->isStartElement()) {
            Q_ASSERT(reader->name()==listedTagName);
            ret[reader->attributes().value("lang").toString()] += reader->readElementText();
        }
    }
    return ret;
}

ApplicationData readApplication(QXmlStreamReader* reader)
{
    Q_ASSERT(reader->isStartElement() && reader->name()=="application");
    ApplicationData ret;
    while(!(reader->isEndElement() && reader->name()=="application")) {
        reader->readNext();
        if(reader->isStartElement()) {
            QStringRef name = reader->name();
            QString lang = reader->attributes().value("lang").toString();
            
            if(name=="pkgname") ret.pkgname = reader->readElementText();
            else if(name=="id") ret.id = reader->readElementText();
            else if(name=="name") ret.name[lang] = reader->readElementText();
            else if(name=="summary") ret.summary[lang] = reader->readElementText();
            else if(name=="icon") ret.icon = reader->readElementText();
            else if(name=="url") ret.url = reader->readElementText();
            else if(name=="keywords") ret.keywords = readElementList(reader, "keyword");
            else if(name=="appcategories") ret.appcategories = joinLists(readElementList(reader, "appcategory").values());
            else if(name=="mimetypes") ret.mimetypes = joinLists(readElementList(reader, "mimetype").values());
            else {
                qWarning() << "unrecognized element:" << reader->name();
            }
            Q_ASSERT(reader->isEndElement());
        } else {;
            Q_ASSERT(reader->isWhitespace() || (reader->isEndElement() && reader->name()=="application"));
        }
    }
    return ret;
}

}

QHash<QString, ApplicationData> AppstreamUtils::fetchAppData(const QString& path)
{
    QHash<QString, ApplicationData> ret;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "couldn't open" << path;
        return ret;
    }

    QXmlStreamReader reader(&f);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == "application") {
            ApplicationData app = readApplication(&reader);
            ret.insert(app.pkgname, app);
        }
    }
    qDebug() << "got a number of appstream datasets:" << ret.size();

    if (reader.hasError()) {
        qWarning() << "error found while parsing" << path << ':' << reader.errorString();
    }
    return ret;
}
