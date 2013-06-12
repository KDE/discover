/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef ORIGINSBACKEND_H
#define ORIGINSBACKEND_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <qdeclarativelist.h>
#include <LibQApt/SourcesList>

class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasSource READ hasSource CONSTANT)
    Q_PROPERTY(QStringList args READ args CONSTANT)
    Q_PROPERTY(QString suite READ suite CONSTANT)
    Q_PROPERTY(QStringList arches READ arches CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled CONSTANT)

    public:
        Entry(QObject* parent, const QApt::SourceEntry &entry)
            : QObject(parent)
            , m_entry(entry) {}
        
        bool hasSource() const { return m_entry.type() == QLatin1String("deb-src"); }
        
        QStringList args() const { return m_entry.components(); }
        
        QString suite() const { return m_entry.dist(); }
        
        QStringList arches() const { return m_entry.architectures(); }
        
        void setEnabled(bool enabled) { m_entry.setEnabled(enabled); }
        bool isEnabled() const { return m_entry.isEnabled(); }
    private:
        QApt::SourceEntry m_entry;
};

class Source : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString uri READ uri CONSTANT)
    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(QDeclarativeListProperty<Entry> entries READ entries CONSTANT)
    public:
        Source(QObject* parent) : QObject(parent) {}
        QString uri() { return m_uri; }
        void setUri(const QString& uri) { m_uri = uri; }
        void addEntry(Entry* entry) { m_entries.append(entry); }
        QDeclarativeListProperty<Entry> entries();
        QString name() const;
        bool enabled() const;
    private:
        QString m_uri;
        QList<Entry*> m_entries;
};

class OriginsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sources READ sourcesVariant NOTIFY originsChanged)
    public:
        explicit OriginsBackend(QObject* parent = 0);
        virtual ~OriginsBackend();

        QVariantList sourcesVariant() const;
        QList<Source*> sources() const { return m_sources; }
        Source* sourceForUri(const QString& uri);

    public slots:
        void addRepository(const QString& repository);
        void removeRepository(const QString& repository);
        void load();

    private slots:
        void additionDone(int processErrorCode);
        void removalDone(int processErrorCode);

    signals:
        void originsChanged();

    private:
        QApt::SourcesList m_sourcesList;
        QList<Source*> m_sources;
};

#endif // ORIGINSBACKEND_H
