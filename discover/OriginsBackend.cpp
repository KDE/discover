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
#include <backends/ApplicationBackend/ApplicationBackend.h>
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

static ApplicationBackend* applicationBackend()
{
    foreach(AbstractResourcesBackend* b, ResourcesModel::global()->backends()) {
        ApplicationBackend* appbackend = qobject_cast<ApplicationBackend*>(b);
        if(qobject_cast<ApplicationBackend*>(b))
            return appbackend;
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
    QApt::Backend* backend = applicationBackend()->backend();
    if(!backend) {
        connect(applicationBackend(), SIGNAL(backendReady()), SLOT(load()));
        return;
    }
    
    qDeleteAll(m_sources);
    m_sources.clear();
    //load /etc/apt/sources.list
    load(backend->config()->findFile("Dir::Etc::sourcelist"));
    
    //load /etc/apt/sources.list.d/*.list
    QDir d(backend->config()->findDirectory("Dir::Etc::sourceparts"));
    foreach(const QString& file, d.entryList(QStringList() << "*.list")) {
        load(d.filePath(file));
    }
}

void OriginsBackend::load(const QString& file)
{
    Q_ASSERT(QFile::exists(file));
    QFile f(file);
    
    if(!f.open(QFile::Text|QFile::ReadOnly))
        return;
    //skip comments and empty lines
    QRegExp rxComment("(\\s*#\\s*)|(^\\s+$)");
    QRegExp rxArchitecture("\\[(.+)\\] ");
    while(!f.atEnd()) {
        QByteArray line = f.readLine();
        int comment = rxComment.indexIn(line);
        bool enabled = true;
        if(comment==0) {
            line = line.mid(rxComment.matchedLength());
            if(!line.startsWith(QByteArray("deb"))) {
                continue;
            }
            enabled = false;
        } else if(comment>0)
            line = line.left(comment);
        
        if(!line.isEmpty()) {
            QString architecture;
            int arch = rxArchitecture.indexIn(line);
            if(arch>=0) {
                architecture = rxArchitecture.cap(1);
                line.remove(arch, rxArchitecture.matchedLength());
            }
            
            QList<QByteArray> source = line.split(' ');
            if(source.count() < 3) {
                return;
            }
            QByteArray uri;
            int opened = 0;
            for(int i=1; i<source.count(); i++) {
                if(source[i].contains('['))
                    ++opened;
                else if(source[i].contains(']'))
                    --opened;
                if(!uri.isEmpty())
                    uri += ' ';
                uri += source[i];
                if(opened==0)
                    break;
            }
            
            Source* newSource = sourceForUri(uri);
            Entry* entry = new Entry(newSource);
            
            entry->setArch(architecture);
            entry->setSource(source.first().endsWith(QByteArray("deb-src")));
            entry->setSuite(source[2]);
            entry->setEnabled(enabled);
            
            QStringList args;
            foreach(const QByteArray& arg, source.mid(3)) {
                args += arg;
            }
            newSource->addEntry(entry);
        }
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
        applicationBackend()->reload();
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
        applicationBackend()->reload();
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
    QStringList origins = applicationBackend()->backend()->originsForHost(uri.host());
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
