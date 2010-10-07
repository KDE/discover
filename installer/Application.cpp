/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "Application.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

// KDE includes
#include <KLocale>
#include <KDebug>

// QApt includes
#include <LibQApt/Package>
#include <LibQApt/Backend>

Application::Application(const QString &fileName, QApt::Backend *backend)
        : m_fileName(fileName)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(true)
{
    m_data = desktopContents();
}

Application::Application(QApt::Package *package)
        : m_package(package)
{
}

Application::~Application()
{
}

QString Application::name()
{
    QString name = getField("Name");
    if (name.isEmpty()) {
        return package()->name();
    }

    return i18n(name.toUtf8());
}

QString Application::comment()
{
    QString comment = getField("Comment");
    if (comment.isEmpty()) {
        // Sometimes GenericName is used instead of Comment
        comment = getField("GenericName");
        if (comment.isEmpty()) {
            return package()->shortDescription();
        }
    }

    return i18n(comment.toUtf8());
}

QApt::Package *Application::package()
{
    if (!m_package) {
        QString packageName = getField("X-AppInstall-Package");
        if (m_backend) {
            m_package = m_backend->package(packageName);
        }
    }

    return m_package;
}

QString Application::icon()
{
    QString icon = getField("Icon");

    return icon;
}

QString Application::categories()
{
    return getField("Categories");
}

int Application::popconScore()
{
    QString popconString = getField("X-AppInstall-Popcon");

    return popconString.toInt();
}

bool Application::isValid()
{
    return m_isValid;
}

QString Application::getField(const QString &field)
{
    return m_data.value(field);
}

QHash<QString, QString> Application::desktopContents()
{
    QHash<QString, QString> contents;
    const QHash<QString, QString> emptyMap;

    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        return emptyMap;
    }

    QTextStream stream(&file);

    QString line = stream.readLine();
    QString lastKey;
    while (!line.isNull()) {
        line = stream.readLine();
        if (line.isEmpty()) {
            continue;
        } else if (line.at(0).isSpace()) {
            line = line.simplified();
            if (line.isEmpty()) {
                continue; // treat it like empty line (lenient)
            }
            if (lastKey.isEmpty()) {
                m_isValid = false;
                return emptyMap; // not a valid desktop file
            }
            QString value = contents[lastKey];
            if (!value.isEmpty())
                value += QLatin1Char(' ');
            contents[lastKey] = value + line;
        } else {
            QStringList splitLine = line.split(QLatin1Char('='));
            if (!(splitLine.size() == 2)) {
                continue;
            }
            QString key = splitLine[0];
            QString value = splitLine[1].simplified();
            contents[key] = value;
            lastKey = key;
        }
    }

    return contents;
}
