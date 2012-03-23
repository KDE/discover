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

class Source : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isSource READ isSource CONSTANT)
    Q_PROPERTY(QString uri READ uri CONSTANT)
    Q_PROPERTY(QStringList args READ args CONSTANT)
    Q_PROPERTY(QString suite READ suite CONSTANT)
    Q_PROPERTY(QString arch READ arch CONSTANT)

    public:
        bool isSource() const { return m_isSource; }
        void setSource(bool s) { m_isSource = s; }
        
        QString uri() { return m_uri; }
        void setUri(const QByteArray& uri) { m_uri = uri; }
        
        QStringList args() const { return m_args; }
        void setArgs(const QStringList& args) { m_args = args; }
        
        QString suite() const { return m_suite; }
        void setSuite(const QByteArray& suite) { m_suite = suite; }
        
        void setArch(const QString& arch) { m_arch = arch; }
        QString arch() const { return m_arch; }
    private:
        bool m_isSource;
        QByteArray m_uri;
        QStringList m_args;
        QByteArray m_suite;
        QString m_arch;
};

class OriginsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sources READ sourcesVariant NOTIFY originsChanged);
    public:
        explicit OriginsBackend(QObject* parent = 0);
        virtual ~OriginsBackend();

        void load();
        void load(const QString& file);
        QVariantList sourcesVariant() const;
        QList<Source*> sources() const { return m_sources; }

    public slots:
        void addRepository(const QString& repository);
        void removeRepository(const QString& repository);
        void initialize();

    private slots:
        void additionDone(int processErrorCode);
        void removalDone(int processErrorCode);

    signals:
        void originsChanged();

    private:
        QList<Source*> m_sources;
};

#endif // ORIGINSBACKEND_H
