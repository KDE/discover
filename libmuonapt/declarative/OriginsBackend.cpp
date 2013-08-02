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

#include "OriginsBackend.h"
#include <resources/ResourcesModel.h>
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QMainWindow>
#include <qdeclarative.h>
#include <LibQApt/Backend>
#include <LibQApt/Config>
#include <KMessageBox>
#include <KLocalizedString>

static QObject* applicationBackend()
{
    foreach(AbstractResourcesBackend* b, ResourcesModel::global()->backends()) {
        if(QByteArray(b->metaObject()->className())=="ApplicationBackend")
            return b;
    }
    return 0;
}

OriginsBackend::OriginsBackend(QObject* parent)
    : QObject(parent)
{
    qmlRegisterType<Source>();
    qmlRegisterType<Entry>();
    load();
}

OriginsBackend::~OriginsBackend()
{
    qDeleteAll(m_sources);
}

void OriginsBackend::load()
{
    QObject* b = applicationBackend();
    QApt::Backend* backend = qobject_cast<QApt::Backend*>(b->property("backend").value<QObject*>());
    if(!backend) {
        connect(b, SIGNAL(fetchingChanged()), SLOT(load()));
        return;
    }
    
    m_sourcesList.reload();

    for (const QApt::SourceEntry &sEntry : m_sourcesList.entries()) {
        if (!sEntry.isValid())
            continue;

        Source* newSource = sourceForUri(sEntry.uri());
        Entry* entry = new Entry(this, sEntry);
        newSource->addEntry(entry);
    }

    emit originsChanged();
}

Source* OriginsBackend::sourceForUri(const QString& uri)
{
    foreach(Source* s, m_sources) {
        if(s->uri()==uri)
            return s;
    }
    Source* s = new Source(this);
    s->setUri(uri);
    m_sources += s;
    return s;
}

void OriginsBackend::addRepository(const QString& repository)
{
    QProcess* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, SIGNAL(finished(int)), SLOT(additionDone(int)));
    connect(p, SIGNAL(finished(int)), p, SLOT(deleteLater()));
    p->start("kdesudo", QStringList("--") << "apt-add-repository" << "-y" << repository);
}

void OriginsBackend::removeRepository(const QString& repository)
{
    QProcess* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, SIGNAL(finished(int)), SLOT(removalDone(int)));
    connect(p, SIGNAL(finished(int)), p, SLOT(deleteLater()));
    p->start("kdesudo", QStringList("--") << "apt-add-repository" << "--remove" << "-y" << repository);
}

void OriginsBackend::additionDone(int processErrorCode)
{
    if(processErrorCode==0) {
        load();
        QMetaObject::invokeMethod(applicationBackend(), "reload");
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        if(errorMessage.isEmpty())
            KMessageBox::error(0, errorMessage, i18n("Adding Origins..."));
    }
}

void OriginsBackend::removalDone(int processErrorCode)
{
    if(processErrorCode==0) {
        load();
        QMetaObject::invokeMethod(applicationBackend(), "reload");
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        if(errorMessage.isEmpty())
            KMessageBox::error(0, errorMessage, i18n("Removing Origins..."));
    }
}

QVariantList OriginsBackend::sourcesVariant() const
{
    QVariantList ret;
    foreach(QObject* source, m_sources) {
        ret += qVariantFromValue<QObject*>(source);
    }
    return ret;
}


bool Source::enabled() const
{
    bool ret = false;
    foreach(Entry* e, m_entries) {
        ret |= e->isEnabled();
    }
    return ret;
}

QDeclarativeListProperty<Entry> Source::entries()
{
    return QDeclarativeListProperty<Entry>(this, m_entries);
}

QString Source::name() const
{
    QUrl uri(m_uri);
    QApt::Backend* backend = qobject_cast<QApt::Backend*>(applicationBackend()->property("backend").value<QObject*>());
    QStringList origins = backend->originsForHost(uri.host());
    if(origins.size()==1)
        return origins.first();
    else if(origins.size()==0)
        return QString();
    else {
        QString path = uri.path();
        int firstSlash = path.indexOf('/', 1);
        int secondSlash = path.indexOf('/', firstSlash+1);
        QString launchpadifyUri = path.mid(1,secondSlash-1).replace('/', '-');
        QStringList results = origins.filter(launchpadifyUri, Qt::CaseInsensitive);
        if(results.isEmpty()) {
            launchpadifyUri = path.mid(1,firstSlash-1).replace('/', '-');
            results = origins.filter(launchpadifyUri, Qt::CaseInsensitive);
        }
        return results.isEmpty() ? QString() : results.first();
    }
}

#include "moc_OriginsBackend.cpp"
