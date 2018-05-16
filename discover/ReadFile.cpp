/*
 *   Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

#include "ReadFile.h"
#include <QRegularExpression>

ReadFile::ReadFile()
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ReadFile::openNow);
    connect(&m_file, &QFile::readyRead, this, &ReadFile::processAll);
}

void ReadFile::setPath(QString path)
{
    processPath(path);
    if (path == m_file.fileName())
        return;

    if (path.isEmpty())
        return;;

    if (m_file.isOpen())
        m_watcher.removePath(m_file.fileName());

    m_file.setFileName(path);
    openNow();

    m_watcher.addPath(m_file.fileName());
}

void ReadFile::openNow()
{
    if (!m_contents.isEmpty()) {
        m_contents.clear();
        Q_EMIT contentsChanged(m_contents);
    }
    m_file.close();
    const auto open = m_file.open(QIODevice::ReadOnly | QIODevice::Text);
    Q_EMIT pathChanged(path());
    if (!open)
        return;

    m_stream.reset(new QTextStream(&m_file));
    process(800);
}

void ReadFile::processPath(QString& path)
{
    const QRegularExpression envRx(QStringLiteral("\\$([A-Z_]+)"));
    auto matchIt = envRx.globalMatch(path);
    while(matchIt.hasNext()) {
        auto match = matchIt.next();
        path.replace(match.capturedStart(), match.capturedLength(), QString::fromUtf8(qgetenv(match.capturedRef(1).toUtf8().constData())));
    }
}

void ReadFile::process(uint max)
{
    QString read = m_stream->readAll();
    if (max>0)
        read = read.right(max);
    m_contents += read;
    Q_EMIT contentsChanged(m_contents);
}
