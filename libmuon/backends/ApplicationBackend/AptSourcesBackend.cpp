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

#include "AptSourcesBackend.h"
#include "ApplicationBackend.h"
#include <QAptActions.h>
#include <qapt/sourceentry.h>
#include <kauthexecutejob.h>
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageWidget>
#include <KMessageBox>
#include <QProcess>

class EntryItem : public QStandardItem
{
public:
    EntryItem(const QApt::SourceEntry &sEntry)
        : m_sEntry(sEntry)
    {}
    QApt::SourceEntry& sourceEntry() { return m_sEntry; }
    
private:
    QApt::SourceEntry m_sEntry;
};

class SourceItem : public QStandardItem
{
public:
    SourceItem(const QUrl& uri)
        : m_uri(uri)
    {}
    
    virtual QVariant data(int role = Qt::UserRole + 1) const;
    QUrl uri() const { return m_uri; }

private:
    QUrl m_uri;
};

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
    m_sources->clear();

    Q_FOREACH (const QApt::SourceEntry &sEntry, m_sourcesList.entries()) {
        if (!sEntry.isValid())
            continue;

        SourceItem* newSource = sourceForUri(sEntry.uri());
        EntryItem* entry = new EntryItem(sEntry);
        newSource->appendRow(entry);
    }
}

SourceItem* AptSourcesBackend::sourceForUri(const QString& uri)
{
    for(int r = 0, c = m_sources->rowCount(); r<c; ++r) {
        SourceItem* s = static_cast<SourceItem*>(m_sources->item(r));
        if(s->uri()==uri)
            return s;
    }
    SourceItem* s = new SourceItem(uri);
    s->setData(uri, UriRole);
    m_sources->appendRow(s);
    return s;
}

QAbstractItemModel* AptSourcesBackend::sources()
{
    return m_sources;
}

bool AptSourcesBackend::removeSource(const QString& repository)
{
    KAuth::Action readAction("org.kde.muon.repo.modify");
    readAction.setHelperId("org.kde.muon.repo");
    QVariantMap args = {
        { "repository", repository },
        { "action", QStringLiteral("remove") }
    };
    readAction.setArguments(args);
    qDebug() << "removing..." << args;
    KAuth::ExecuteJob* reply = readAction.execute();
    removalDone(reply->error());
    return true;
}

bool AptSourcesBackend::addSource(const QString& repository)
{
    KAuth::Action readAction("org.kde.muon.repo.modify");
    readAction.setHelperId("org.kde.muon.repo");
    QVariantMap args = {
        { "repository", repository },
        { "action", QStringLiteral("add") }
    };
    readAction.setArguments(args);
    qDebug() << "adding..." << args;
    KAuth::ExecuteJob* reply = readAction.execute();
    additionDone(reply->error());
    return true;
}

void AptSourcesBackend::additionDone(int processErrorCode)
{
    if(processErrorCode==0) {
        load();
        QMetaObject::invokeMethod(appsBackend(), "reload");
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        if(!errorMessage.isEmpty())
            KMessageBox::error(0, errorMessage, i18n("Adding Origins..."));
    }
}

void AptSourcesBackend::removalDone(int processErrorCode)
{
    if(processErrorCode==0) {
        load();
        QMetaObject::invokeMethod(appsBackend(), "reload");
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        if(!errorMessage.isEmpty())
            KMessageBox::error(0, errorMessage, i18n("Removing Origins..."));
    }
}

ApplicationBackend* AptSourcesBackend::appsBackend() const
{
    return qobject_cast<ApplicationBackend*>(parent());
}

QVariant SourceItem::data(int role) const
{
    switch(role) {
        case Qt::DisplayRole: {
//             modelData.name=="" ? modelData.uri : i18n("%1. %2", modelData.name, modelData.uri)
            QApt::Backend* backend = qobject_cast<AptSourcesBackend*>(model()->parent())->appsBackend()->backend();
            QStringList origins = !m_uri.host().isEmpty() ? backend->originsForHost(m_uri.host()) : QStringList();
            
            if(origins.size()==1)
                return origins.first();
            else if(origins.size()==0)
                return m_uri.toDisplayString();
            else {
                QString path = m_uri.path();
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
            for(int i=0, c=rowCount(); i<c; ++i) {
                EntryItem* entry = static_cast<EntryItem*>(child(i));
                
                QString suite = entry->sourceEntry().dist();
                if(!vals.contains(suite))
                    vals[suite]=0;

                bool hasSource = entry->sourceEntry().type() == QLatin1String("deb-src");
                if(hasSource)
                    vals[suite] += 2;
                else
                    vals[suite] += 1;
            }
            QStringList ret;
            Q_FOREACH (const QString& e, vals.keys()) {
                if(vals[e]>1)
                    ret.append(e);
                else
                    ret.append(i18n("%1 (Binary)", e));
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

QString AptSourcesBackend::name() const
{
    return i18n("Software Management");
}

QList<QAction*> AptSourcesBackend::actions() const
{
    return QList<QAction*>() << QAptActions::self()->actionCollection()->action("software_properties");
}
