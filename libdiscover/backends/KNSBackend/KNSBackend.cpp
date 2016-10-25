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

// Qt includes
#include <QDebug>
#include <QDir>
#include <QFileInfo>

// Attica includes
#include <attica/content.h>
#include <attica/providermanager.h>

// KDE includes
#include <kns3/downloadmanager.h>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

// DiscoverCommon includes
#include "Transaction/Transaction.h"
#include "Transaction/TransactionModel.h"
#include "Category/Category.h"

// Own includes
#include "KNSBackend.h"
#include "KNSResource.h"
#include "KNSReviews.h"
#include <resources/StandardBackendUpdater.h>

MUON_BACKEND_PLUGIN(KNSBackend)

KNSBackend::KNSBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_fetching(false)
    , m_isValid(true)
    , m_page(0)
    , m_reviews(new KNSReviews(this))
    , m_updater(new StandardBackendUpdater(this))
{}

KNSBackend::~KNSBackend() = default;

void KNSBackend::markInvalid(const QString &message)
{
    qWarning() << "invalid kns backend!" << m_name << "because:" << message;
    m_isValid = false;
    setFetching(false);
}

void KNSBackend::setMetaData(const QString& path)
{
    KDesktopFile cfg(path);
    KConfigGroup service = cfg.group("Desktop Entry");

    m_iconName = service.readEntry("Icon", QString());

    const QString knsrc = service.readEntry("X-Muon-Arguments", QString());
    m_name = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, knsrc);
    if (m_name.isEmpty()) {
        QString p = QFileInfo(path).dir().filePath(knsrc);
        if (QFile::exists(p))
            m_name = p;

    }

    if (m_name.isEmpty()) {
        markInvalid(QStringLiteral("Couldn't find knsrc file: ") + knsrc);
        return;
    }

    const KConfig conf(m_name);
    if (!conf.hasGroup("KNewStuff3")) {
        markInvalid(QStringLiteral("Config group not found! Check your KNS3 installation."));
        return;
    }

    m_categories = QStringList{ QFileInfo(m_name).fileName() };

    const KConfigGroup group = conf.group("KNewStuff3");
    m_extends = group.readEntry("Extends", QStringList());
    m_reviews->setProviderUrl(QUrl(group.readEntry("ProvidersUrl", QString())));

    setFetching(true);

    m_manager = new KNS3::DownloadManager(m_name, this);
    connect(m_manager, &KNS3::DownloadManager::errorFound, this, [](const QString &error) { qWarning() << "kns error" << error; });
    connect(m_manager, &KNS3::DownloadManager::searchResult, this, &KNSBackend::receivedEntries);
    connect(m_manager, &KNS3::DownloadManager::entryStatusChanged, this, &KNSBackend::statusChanged);
    m_page = 0;
    m_manager->checkForInstalled();
}

void KNSBackend::setFetching(bool f)
{
    if(m_fetching!=f) {
        m_fetching = f;
        emit fetchingChanged();
    }
}

bool KNSBackend::isValid() const
{
    return m_isValid;
}

void KNSBackend::receivedEntries(const KNS3::Entry::List& entries)
{
    if(entries.isEmpty()) {
        Q_EMIT searchFinished();
        setFetching(false);
        return;
    }

    QVector<AbstractResource*> resources;
    resources.reserve(entries.count());
    foreach(const KNS3::Entry& entry, entries) {
        KNSResource* r = new KNSResource(entry, m_categories, this);
        m_resourcesByName.insert(entry.id(), r);
        resources += r;
    }
    Q_EMIT receivedResources(resources);
    ++m_page;
    m_manager->search(m_page);
}

void KNSBackend::statusChanged(const KNS3::Entry& entry)
{
    KNSResource* r = qobject_cast<KNSResource*>(m_resourcesByName.value(entry.id()));
    if(r)
        r->setEntry(entry);
    else
        qWarning() << "unknown entry changed" << entry.id() << entry.name();
}

class KNSTransaction : public Transaction
{
public:
    KNSTransaction(QObject* parent, KNSResource* res, Transaction::Role role)
        : Transaction(parent, res, role)
        , m_id(res->entry().id())
    {
        TransactionModel::global()->addTransaction(this);

        setCancellable(false);

        auto manager = res->knsBackend()->downloadManager();
        connect(manager, &KNS3::DownloadManager::entryStatusChanged, this, &KNSTransaction::anEntryChanged);
    }

    void anEntryChanged(const KNS3::Entry& entry) {
        if (entry.id() == m_id) {
            switch (entry.status()) {
                case KNS3::Entry::Invalid:
                    qWarning() << "invalid status for" << entry.id() << entry.status();
                    break;
                case KNS3::Entry::Installing:
                case KNS3::Entry::Updating:
                    setStatus(CommittingStatus);
                    break;
                case KNS3::Entry::Downloadable:
                case KNS3::Entry::Installed:
                case KNS3::Entry::Deleted:
                case KNS3::Entry::Updateable:
                    setStatus(DoneStatus);
                    TransactionModel::global()->removeTransaction(this);
                    break;
            }
        }
    }

    ~KNSTransaction() override {
        if (TransactionModel::global()->contains(this)) {
            qWarning() << "deleting Transaction before it's done";
                TransactionModel::global()->removeTransaction(this);
        }
    }

    void cancel() override {}

private:
    const QString m_id;
};

void KNSBackend::removeApplication(AbstractResource* app)
{
    auto res = qobject_cast<KNSResource*>(app);
    m_manager->uninstallEntry(res->entry());
    new KNSTransaction(this, res, Transaction::RemoveRole);
}

void KNSBackend::installApplication(AbstractResource* app)
{
    auto res = qobject_cast<KNSResource*>(app);
    m_manager->installEntry(res->entry());
    new KNSTransaction(this, res, Transaction::InstallRole);
}

void KNSBackend::installApplication(AbstractResource* app, const AddonList& /*addons*/)
{
    installApplication(app);
}

int KNSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractReviewsBackend* KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

ResultsStream* KNSBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    if (filter.state >= AbstractResource::Installed) {
        QVector<AbstractResource*> ret;
        foreach(AbstractResource* r, m_resourcesByName) {
            if(r->state()>=filter.state && (r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive)))
                ret += r;
        }
        return new ResultsStream(QStringLiteral("KNS-installed"), ret);
    } else if (filter.category && filter.category->matchesCategoryName(m_categories.first())) {
        m_manager->setSearchTerm(filter.search);
        return searchStream();
    } else if (!filter.search.isEmpty()) {
        m_manager->setSearchTerm(filter.search);
        return searchStream();
    }
    return new ResultsStream(QStringLiteral("KNS-void"), {});
}

ResultsStream * KNSBackend::searchStream()
{
    m_manager->search(0);
    m_page = 0;
    auto stream = new ResultsStream(QStringLiteral("KNS-search"));
    connect(this, &KNSBackend::receivedResources, stream, &ResultsStream::resourcesFound);
    connect(this, &KNSBackend::searchFinished, stream, &ResultsStream::deleteLater);
    return stream;
}

ResultsStream * KNSBackend::findResourceByPackageName(const QString& search)
{
    auto pkg = m_resourcesByName.value(search);
    return new ResultsStream(QStringLiteral("KNS"), pkg ? QVector<AbstractResource*>{pkg} : QVector<AbstractResource*>{});
}

bool KNSBackend::isFetching() const
{
    return m_fetching;
}

AbstractBackendUpdater* KNSBackend::backendUpdater() const
{
    return m_updater;
}

#include "KNSBackend.moc"
