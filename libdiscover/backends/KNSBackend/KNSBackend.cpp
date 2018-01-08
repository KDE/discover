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
#include <QStandardPaths>
#include <QTimer>
#include <QDirIterator>

// Attica includes
#include <attica/content.h>
#include <attica/providermanager.h>

// KDE includes
#include <knewstuffcore_version.h>
#include <KNSCore/Engine>
#include <KNSCore/QuestionManager>
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
#include "utils.h"

class KNSBackendFactory : public AbstractResourcesBackendFactory {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.muon.AbstractResourcesBackendFactory")
    Q_INTERFACES(AbstractResourcesBackendFactory)
    public:
        KNSBackendFactory() {
            connect(KNSCore::QuestionManager::instance(), &KNSCore::QuestionManager::askQuestion, this, [](KNSCore::Question* q) {
                qWarning() << q->question() << q->questionType();
                q->setResponse(KNSCore::Question::InvalidResponse);
            });
        }

        QVector<AbstractResourcesBackend*> newInstance(QObject* parent, const QString &/*name*/) const override
        {
            QVector<AbstractResourcesBackend*> ret;
            for (const QString &path: QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)) {
                QDirIterator dirIt(path, {QStringLiteral("*.knsrc")}, QDir::Files);
                for(; dirIt.hasNext(); ) {
                    dirIt.next();

                    auto bk = new KNSBackend(parent, QStringLiteral("plasma"), dirIt.filePath());
                    ret += bk;
                }
            }
            return ret;
        }
};

Q_DECLARE_METATYPE(KNSCore::EntryInternal)

KNSBackend::KNSBackend(QObject* parent, const QString& iconName, const QString &knsrc)
    : AbstractResourcesBackend(parent)
    , m_fetching(false)
    , m_isValid(true)
    , m_reviews(new KNSReviews(this))
    , m_name(knsrc)
    , m_iconName(iconName)
    , m_updater(new StandardBackendUpdater(this))
{
    const QString fileName = QFileInfo(m_name).fileName();
    setName(fileName);
    setObjectName(knsrc);

    const KConfig conf(m_name);
    if (!conf.hasGroup("KNewStuff3")) {
        markInvalid(QStringLiteral("Config group not found! Check your KNS3 installation."));
        return;
    }

    m_categories = QStringList{ fileName };

    const KConfigGroup group = conf.group("KNewStuff3");
    m_extends = group.readEntry("Extends", QStringList());
    m_reviews->setProviderUrl(QUrl(group.readEntry("ProvidersUrl", QString())));

    setFetching(true);

    m_engine = new KNSCore::Engine(this);
    m_engine->init(m_name);
#if KNEWSTUFFCORE_VERSION_MAJOR==5 && KNEWSTUFFCORE_VERSION_MINOR>=36
    m_engine->setPageSize(100);
#endif
    // Setting setFetching to false when we get an error ensures we don't end up in an eternally-fetching state
    connect(m_engine, &KNSCore::Engine::signalError, this, [this](const QString &_error) {
        QString error = _error;
        if(error == QLatin1Literal("All categories are missing")) {
            markInvalid(error);
            error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
        }
        m_responsePending = false;
        Q_EMIT searchFinished();
        Q_EMIT availableForQueries();
        this->setFetching(false);
        qWarning() << "kns error" << objectName() << error;
        passiveMessage(i18n("%1: %2", name(), error));
    });
    connect(m_engine, &KNSCore::Engine::signalEntriesLoaded, this, &KNSBackend::receivedEntries, Qt::QueuedConnection);
    connect(m_engine, &KNSCore::Engine::signalEntryChanged, this, &KNSBackend::statusChanged, Qt::QueuedConnection);
    connect(m_engine, &KNSCore::Engine::signalEntryDetailsLoaded, this, &KNSBackend::statusChanged);
    connect(m_engine, &KNSCore::Engine::signalProvidersLoaded, this, &KNSBackend::fetchInstalled);

    const QVector<QPair<FilterType, QString>> filters = { {CategoryFilter, fileName } };
    const QSet<QString> backendName = { name() };
    m_displayName = group.readEntry("Name", QString());
    if (m_displayName.isEmpty()) {
        m_displayName = fileName.mid(0, fileName.indexOf(QLatin1Char('.')));
        m_displayName[0] = m_displayName[0].toUpper();
    }

    static const QSet<QString> knsrcPlasma = {
        QStringLiteral("aurorae.knsrc"), QStringLiteral("icons.knsrc"), QStringLiteral("kfontinst.knsrc"), QStringLiteral("lookandfeel.knsrc"), QStringLiteral("plasma-themes.knsrc"), QStringLiteral("plasmoids.knsrc"),
        QStringLiteral("wallpaper.knsrc"), QStringLiteral("xcursor.knsrc"),

        QStringLiteral("cgcgtk3.knsrc"), QStringLiteral("cgcicon.knsrc"), QStringLiteral("cgctheme.knsrc"), //GTK integration
        QStringLiteral("kwinswitcher.knsrc"), QStringLiteral("kwineffect.knsrc"), QStringLiteral("kwinscripts.knsrc") //KWin
    };
    auto actualCategory = new Category(m_displayName, QStringLiteral("plasma"), filters, backendName, {}, QUrl(), true);

    const auto topLevelName = knsrcPlasma.contains(fileName)? i18n("Plasma Addons") : i18n("Application Addons");
    const QUrl decoration(knsrcPlasma.contains(fileName)? QStringLiteral("https://c2.staticflickr.com/4/3148/3042248532_20bd2e38f4_b.jpg") : QStringLiteral("https://c2.staticflickr.com/8/7067/6847903539_d9324dcd19_o.jpg"));
    auto addonsCategory = new Category(topLevelName, QStringLiteral("plasma"), filters, backendName, {actualCategory}, decoration, true);
    m_rootCategories = { addonsCategory };
}

KNSBackend::~KNSBackend() = default;

void KNSBackend::markInvalid(const QString &message)
{
    qWarning() << "invalid kns backend!" << m_name << "because:" << message;
    m_isValid = false;
    setFetching(false);
}

void KNSBackend::fetchInstalled()
{
    auto search = new OneTimeAction([this]() {
        Q_EMIT startingSearch();
        m_onePage = true;
        m_responsePending = true;
        m_engine->checkForInstalled();
    }, this);

    if (m_responsePending) {
        connect(this, &KNSBackend::availableForQueries, search, &OneTimeAction::trigger, Qt::QueuedConnection);
    } else {
        search->trigger();
    }
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

KNSResource* KNSBackend::resourceForEntry(const KNSCore::EntryInternal& entry)
{
    KNSResource* r = static_cast<KNSResource*>(m_resourcesByName.value(entry.uniqueId()));
    if (!r) {
        r = new KNSResource(entry, m_categories, this);
        m_resourcesByName.insert(entry.uniqueId(), r);
    } else {
        r->setEntry(entry);
    }
    return r;
}

void KNSBackend::receivedEntries(const KNSCore::EntryInternal::List& entries)
{
    m_responsePending = false;

    const auto resources = kTransform<QVector<AbstractResource*>>(entries, [this](const KNSCore::EntryInternal& entry){ return resourceForEntry(entry); });
    if (!resources.isEmpty()) {
        Q_EMIT receivedResources(resources);
    }

    if(resources.isEmpty()) {
        Q_EMIT searchFinished();
        Q_EMIT availableForQueries();
        setFetching(false);
        return;
    }
//     qDebug() << "received" << objectName() << this << m_resourcesByName.count();
    if (!m_responsePending && !m_onePage) {
        // We _have_ to set this first. If we do not, we may run into a situation where the
        // data request will conclude immediately, causing m_responsePending to remain true
        // for perpetuity as the slots will be called before the function returns.
        m_responsePending = true;
        m_engine->requestMoreData();
    } else {
        Q_EMIT availableForQueries();
    }
}

void KNSBackend::statusChanged(const KNSCore::EntryInternal& entry)
{
    resourceForEntry(entry);
}

class KNSTransaction : public Transaction
{
public:
    KNSTransaction(QObject* parent, KNSResource* res, Transaction::Role role)
        : Transaction(parent, res, role)
        , m_id(res->entry().uniqueId())
    {
        setCancellable(false);

        auto manager = res->knsBackend()->engine();
        connect(manager, &KNSCore::Engine::signalEntryChanged, this, &KNSTransaction::anEntryChanged);
        TransactionModel::global()->addTransaction(this);
    }

    void anEntryChanged(const KNSCore::EntryInternal& entry) {
        if (entry.uniqueId() == m_id) {
            switch (entry.status()) {
                case KNS3::Entry::Invalid:
                    qWarning() << "invalid status for" << entry.uniqueId() << entry.status();
                    break;
                case KNS3::Entry::Installing:
                case KNS3::Entry::Updating:
                    setStatus(CommittingStatus);
                    break;
                case KNS3::Entry::Downloadable:
                case KNS3::Entry::Installed:
                case KNS3::Entry::Deleted:
                case KNS3::Entry::Updateable:
                    if (status() != DoneStatus) {
                        setStatus(DoneStatus);
                    }
                    break;
            }
        }
    }

    void cancel() override {}

private:
    const QString m_id;
};

Transaction* KNSBackend::removeApplication(AbstractResource* app)
{
    auto res = qobject_cast<KNSResource*>(app);
    auto t = new KNSTransaction(this, res, Transaction::RemoveRole);
    m_engine->uninstall(res->entry());
    return t;
}

Transaction* KNSBackend::installApplication(AbstractResource* app)
{
    auto res = qobject_cast<KNSResource*>(app);
    m_engine->install(res->entry());
    return new KNSTransaction(this, res, Transaction::InstallRole);
}

Transaction* KNSBackend::installApplication(AbstractResource* app, const AddonList& /*addons*/)
{
    return installApplication(app);
}

int KNSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractReviewsBackend* KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

static ResultsStream* voidStream()
{
    return new ResultsStream(QStringLiteral("KNS-void"), {});
}

ResultsStream* KNSBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    if (!m_isValid || (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() != QLatin1String("kns")) || !filter.mimetype.isEmpty())
        return voidStream();

    if (filter.resourceUrl.scheme() == QLatin1String("kns")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (filter.state >= AbstractResource::Installed) {
        QVector<AbstractResource*> ret;
        foreach(AbstractResource* r, m_resourcesByName) {
            if(r->state()>=filter.state && (r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive)))
                ret += r;
        }
        return new ResultsStream(QStringLiteral("KNS-installed"), ret);
    } else if (filter.category && filter.category->matchesCategoryName(m_categories.constFirst())) {
        return searchStream(filter.search);
    } else if (!filter.category && !filter.search.isEmpty()) {
        return searchStream(filter.search);
    }
    return voidStream();
}

ResultsStream* KNSBackend::searchStream(const QString &searchText)
{
    Q_EMIT startingSearch();

    auto stream = new ResultsStream(QStringLiteral("KNS-search-")+name());
    auto start = [this, stream, searchText]() {
        // No need to explicitly launch a search, setting the search term already does that for us
        m_engine->setSearchTerm(searchText);
        m_onePage = false;
        m_responsePending = true;

        connect(this, &KNSBackend::receivedResources, stream, &ResultsStream::resourcesFound);
        connect(this, &KNSBackend::searchFinished, stream, &ResultsStream::finish);
        connect(this, &KNSBackend::startingSearch, stream, &ResultsStream::finish);
    };

    if (m_responsePending) {
        connect(this, &KNSBackend::availableForQueries, stream, start, Qt::QueuedConnection);
    } else {
        start();
    }
    return stream;
}

ResultsStream * KNSBackend::findResourceByPackageName(const QUrl& search)
{
    if (search.scheme() != QLatin1String("kns") || search.host() != name())
        return voidStream();

    const auto pathParts = search.path().split(QLatin1Char('/'), QString::SkipEmptyParts);
    if (pathParts.size() != 2) {
        passiveMessage(i18n("Wrong KNewStuff URI: %1", search.toString()));
        return voidStream();
    }
    const auto providerid = pathParts.at(0);
    const auto entryid = pathParts.at(1);

    auto stream = new ResultsStream(QStringLiteral("KNS-byname-")+entryid);
    auto start = [this, entryid, stream, providerid]() {
        m_responsePending = true;
        m_engine->fetchEntryById(entryid);
        m_onePage = false;
        connect(m_engine, &KNSCore::Engine::signalError, stream, &ResultsStream::finish);
        connect(m_engine, &KNSCore::Engine::signalEntryDetailsLoaded, stream, [this, stream, entryid, providerid](const KNSCore::EntryInternal &entry) {
            if (entry.uniqueId() == entryid && providerid == QUrl(entry.providerId()).host()) {
                stream->resourcesFound({resourceForEntry(entry)});
            }
            m_responsePending = false;
            QTimer::singleShot(0, this, &KNSBackend::availableForQueries);
            stream->finish();
        });
    };
    if (m_responsePending) {
        connect(this, &KNSBackend::availableForQueries, stream, start);
    } else {
        start();
    }
    return stream;
}

bool KNSBackend::isFetching() const
{
    return m_fetching;
}

AbstractBackendUpdater* KNSBackend::backendUpdater() const
{
    return m_updater;
}

QString KNSBackend::displayName() const
{
    return QStringLiteral("KNewStuff");
}

#include "KNSBackend.moc"
