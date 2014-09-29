/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "aptsourcesbackend.h"

AptSourcesBackend::AptSourcesBackend(ApplicationBackend* backend)
    : AbstractSourcesBackend(backend)
    , m_sources(new QStandardItemModel(this))
{
    connect(backend, SIGNAL(fetchingChanged()), SLOT(load()), Qt::UniqueConnection);
    if (!backend->isFetching()) {
        load();
    }
}

void AptSourcesBackend::load()
{
    m_sourcesList.reload();
    qDeleteAll(m_sources);
    m_sources.clear();

    for (const QApt::SourceEntry &sEntry : m_sourcesList.entries()) {
        if (!sEntry.isValid())
            continue;

        Source* newSource = sourceForUri(sEntry.uri());
        Entry* entry = new Entry(this, sEntry);
        newSource->addEntry(entry);
    }

    emit originsChanged();
}

Source* AptSourcesBackend::sourceForUri(const QString& uri)
{
    foreach(SourceItem* s, m_sources) {
        if(s->uri()==uri)
            return s;
    }
    Source* s = new Source(this);
    s->setUri(uri);
    m_sources->addRow(s);
    return s;
}

QAbstractItemModel* AptSourcesBackend::sources()
{
    return m_sources;
}

bool AptSourcesBackend::removeSource()
{
    KAuth::Action readAction("org.kde.muon.repo.modify");
    readAction.setHelperId("org.kde.muon.repo");
    QVariantMap args;
    args["repository"] = repository;
    args["action"] = QString("remove");
    readAction.setArguments(args);
    KAuth::ExecuteJob* reply = readAction.execute();
    removalDone(reply->error());
}

bool AptSourcesBackend::addSource()
{
    KAuth::Action readAction("org.kde.muon.repo.modify");
    readAction.setHelperId("org.kde.muon.repo");
    QVariantMap args;
    args["repository"] = repository;
    args["action"] = QString("add");
    readAction.setArguments(args);
    KAuth::ExecuteJob* reply = readAction.execute();
    additionDone(reply->error());
}

void AptSourcesBackend::additionDone(int processErrorCode)
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

void AptSourcesBackend::removalDone(int processErrorCode)
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


QVariant SourceItem::data(int role) const
{
    switch(role) {
        case Qt::DisplayRole: {
//             modelData.name=="" ? modelData.uri : i18n("%1. %2", modelData.name, modelData.uri)
            QUrl uri(m_uri);
            QApt::Backend* backend = qobject_cast<QApt::Backend*>(parent());
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
        case Qt::ToolTipRole: {
            QMap<QString, int> vals;
            for (QStandardItem* entry : children()) {
                QString suite = it->data(Suite);
                if(vals[]==null)
                    vals[suite]=0;

                if(it->data(IsSource))
                    vals[suite] += 2
                else
                    vals[suite] += 1
            }
            QStringList ret;
            for(const QString& key, vals.keys()) {
                if(vals[e]>1)
                    ret.push(e)
                else
                    ret.push(i18n("%1 (Binary)", e))
            }

            return ret.join(", ");
        }
        default:
            return QStandardItem::data(role);
    }
}

QString AptSourcesBackend::idDescription()
{
    return i18n(  "<sourceline> - The apt repository source line to add. This is one of:\n"
                        "  a complete apt line, \n"
                        "  a repo url and areas (areas defaults to 'main')\n"
                        "  a PPA shortcut.\n\n"

                        "  Examples:\n"
                        "    deb http://myserver/path/to/repo stable myrepo\n"
                        "    http://myserver/path/to/repo myrepo\n"
                        "    https://packages.medibuntu.org free non-free\n"
                        "    http://extras.ubuntu.com/ubuntu\n"
                        "    ppa:user/repository");
}

