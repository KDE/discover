/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QFile>
#include <QFileSystemWatcher>
#include <QQmlParserStatus>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QTextStream>

class ReadFile : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString contents READ contents NOTIFY contentsChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter FINAL)
public:
    ReadFile();

    QString contents() const
    {
        return m_contents;
    }
    QString path() const
    {
        return m_file.fileName();
    }
    void setPath(QString path);

    QString filter() const;
    void setFilter(const QString &filter);

    void classBegin() override
    {
    }
    void componentComplete() override;

Q_SIGNALS:
    void pathChanged(const QString &path);
    void contentsChanged(const QString &contents);

private:
    void process();
    void openNow();
    void processPath(QString &path);

    bool completed = false;
    QFile m_file;
    QString m_contents;
    QSharedPointer<QTextStream> m_stream;
    QFileSystemWatcher m_watcher;
    QRegularExpression m_filter;
    qint64 m_sizeOnSet = 0;
};
