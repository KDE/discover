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

#ifndef READFILE_H
#define READFILE_H

#include <QFile>
#include <QTextStream>
#include <QSharedPointer>
#include <QFileSystemWatcher>

class ReadFile : public QObject
{
Q_OBJECT
Q_PROPERTY(QString contents READ contents NOTIFY contentsChanged)
Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
public:
    ReadFile();

    QString contents() const { return m_contents; }
    QString path() const { return m_file.fileName(); }
    void setPath(QString path);


Q_SIGNALS:
    void pathChanged(const QString &path);
    void contentsChanged(const QString &contents);

private:
    void processAll() { return process(0); }
    void process(uint max);
    void openNow();
    void processPath(QString& path);

    QFile m_file;
    QString m_contents;
    QSharedPointer<QTextStream> m_stream;
    QFileSystemWatcher m_watcher;
};

#endif // READFILE_H
