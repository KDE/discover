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

    const KConfigGroup group = conf.group("KNewStuff3");
    m_extends = group.readEntry("Extends", QStringList());
    m_reviews->setProviderUrl(QUrl(group.readEntry("ProvidersUrl", QString())));

    setFetching(true);

    m_manager = new KNS3::DownloadManager(m_name, this);
    connect(m_manager, &KNS3::DownloadManager::errorFound, this, &KNSBackend::markInvalid);
    connect(m_manager, &KNS3::DownloadManager::searchResult, this, &KNSBackend::receivedEntries);
    connect(m_manager, &KNS3::DownloadManager::entryStatusChanged, this, &KNSBackend::statusChanged);

    m_page = 0;
    m_manager->search(m_page);
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
        setFetching(false);
        return;
    }

    const QString filename = QFileInfo(m_name).fileName();
    foreach(const KNS3::Entry& entry, entries) {
        KNSResource* r = new KNSResource(entry, filename, this);
        m_resourcesByName.insert(entry.id(), r);
    }
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

class LambdaTransaction : public Transaction
{
public:
    LambdaTransaction(QObject* parent, AbstractResource* res, Transaction::Role role)
        : Transaction(parent, res, role)
    {
        setCancellable(false);
        TransactionModel::global()->addTransaction(this);
    }

    ~LambdaTransaction() override {
        TransactionModel::global()->removeTransaction(this);
    }

    void cancel() override {}
};

void KNSBackend::removeApplication(AbstractResource* app)
{
    QScopedPointer<Transaction> t(new LambdaTransaction(this, app, Transaction::RemoveRole));
    KNSResource* r = qobject_cast<KNSResource*>(app);
    m_manager->uninstallEntry(r->entry());
}

void KNSBackend::installApplication(AbstractResource* app)
{
    QScopedPointer<Transaction> t(new LambdaTransaction(this, app, Transaction::InstallRole));
    KNSResource* r = qobject_cast<KNSResource*>(app);
    m_manager->installEntry(r->entry());
}

void KNSBackend::installApplication(AbstractResource* app, const AddonList& /*addons*/)
{
    installApplication(app);
}

AbstractResource* KNSBackend::resourceByPackageName(const QString& name) const
{
    return m_resourcesByName[name];
}

int KNSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractReviewsBackend* KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

QList<AbstractResource*> KNSBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->name().contains(searchText, Qt::CaseInsensitive) || r->comment().contains(searchText, Qt::CaseInsensitive))
            ret += r;
    }
    return ret;
}

QVector<AbstractResource*> KNSBackend::allResources() const
{
    return containerValues<QVector<AbstractResource*>>(m_resourcesByName);
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
