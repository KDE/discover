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
#include "discover_debug.h"

ReadFile::ReadFile()
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ReadFile::openNow);
    connect(&m_file, &QFile::readyRead, this, &ReadFile::process);
}

void ReadFile::componentComplete()
{
    completed = true;
    openNow();
}

void ReadFile::setPath(QString path)
{
    processPath(path);
    if (path == m_file.fileName())
        return;

    if (path.isEmpty())
        return;

    if (m_file.isOpen())
        m_watcher.removePath(m_file.fileName());

    m_file.setFileName(path);
    m_sizeOnSet = m_file.size() + 1;
    openNow();

    m_watcher.addPath(m_file.fileName());
}

void ReadFile::openNow()
{
    if (!completed)
        return;

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
    m_stream->seek(m_sizeOnSet);
    process();
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

void ReadFile::process()
{
    const QString read = m_stream->readAll();

    if (m_filter.isValid() && !m_filter.pattern().isEmpty()) {
        auto it = m_filter.globalMatch(read);
        while(it.hasNext()) {
            const auto match = it.next();
            m_contents.append(match.capturedRef(match.lastCapturedIndex()));
            m_contents.append(QLatin1Char('\n'));
        }
    } else
        m_contents += read;
    Q_EMIT contentsChanged(m_contents);
}

void ReadFile::setFilter(const QString& filter)
{
    m_filter = QRegularExpression(filter);
    if (!m_filter.isValid())
        qCDebug(DISCOVER_LOG) << "error" << m_filter.errorString();
    Q_ASSERT(filter.isEmpty() || m_filter.isValid());
}

QString ReadFile::filter() const
{
    return m_filter.pattern();
}

