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

class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasSource READ hasSource CONSTANT)
    Q_PROPERTY(QStringList args READ args CONSTANT)
    Q_PROPERTY(QString suite READ suite CONSTANT)
    Q_PROPERTY(QString arch READ arch CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled CONSTANT)

    public:
        Entry(QObject* parent) : QObject(parent), m_isSource(false) {}
        
        bool hasSource() const { return m_isSource; }
        void setSource(bool s) { m_isSource = s; }
        
        QStringList args() const { return m_args; }
        void setArgs(const QStringList& args) { m_args = args; }
        
        QString suite() const { return m_suite; }
        void setSuite(const QByteArray& suite) { m_suite = suite; }
        
        void setArch(const QString& arch) { m_arch = arch; }
        QString arch() const { return m_arch; }
        
        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }
    private:
        bool m_enabled;
        bool m_isSource;
        QStringList m_args;
        QByteArray m_suite;
        QString m_arch;
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
    Q_PROPERTY(QVariantList sources READ sourcesVariant NOTIFY originsChanged);
    public:
        explicit OriginsBackend(QObject* parent = 0);
        virtual ~OriginsBackend();

        void load(const QString& file);
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
        QList<Source*> m_sources;
};

#endif // ORIGINSBACKEND_H
