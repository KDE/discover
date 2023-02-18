/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// Qt includes
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

// Attica includes
#include <Attica/Content>
#include <Attica/ProviderManager>

// KDE includes
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNSCore/Engine>
#include <KNSCore/QuestionManager>
#include <knewstuffcore_version.h>

// DiscoverCommon includes
#include "Category/Category.h"
#include "Transaction/Transaction.h"
#include "Transaction/TransactionModel.h"

// Own includes
#include "KNSBackend.h"
#include "KNSResource.h"
#include "KNSReviews.h"
#include "KNSTransaction.h"
#include "utils.h"
#include <resources/StandardBackendUpdater.h>

static const int ENGINE_PAGE_SIZE = 100;

class KNSBackendFactory : public AbstractResourcesBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.muon.AbstractResourcesBackendFactory")
    Q_INTERFACES(AbstractResourcesBackendFactory)
public:
    KNSBackendFactory()
    {
        connect(KNSCore::QuestionManager::instance(), &KNSCore::QuestionManager::askQuestion, this, [](KNSCore::Question *question) {
            const auto transactions = TransactionModel::global()->transactions();
            for (auto t : transactions) {
                const auto transaction = dynamic_cast<KNSTransaction *>(t);
                if (!transaction) {
                    continue;
                }

                if (question->entry().uniqueId() == transaction->uniqueId()) {
                    switch (question->questionType()) {
                    case KNSCore::Question::ContinueCancelQuestion:
                        transaction->addQuestion(question);
                        return;
                    default:
                        transaction->passiveMessage(i18n("Unsupported question:\n%1", question->question()));
                        question->setResponse(KNSCore::Question::InvalidResponse);
                        transaction->setStatus(Transaction::CancelledStatus);
                        break;
                    }
                    return;
                }
            }
            qWarning() << "Question for unknown resource" << question->question() << question->questionType();
            question->setResponse(KNSCore::Question::InvalidResponse);
        });
    }

    QVector<AbstractResourcesBackend *> newInstance(QObject *parent, const QString & /*name*/) const override
    {
        QVector<AbstractResourcesBackend *> ret;
        const QStringList availableConfigFiles = KNSCore::Engine::availableConfigFiles();
        for (const QString &configFile : availableConfigFiles) {
            auto bk = new KNSBackend(parent, QStringLiteral("plasma"), configFile);
            if (bk->isValid())
                ret += bk;
            else
                delete bk;
        }
        return ret;
    }
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE(KNSCore::EntryInternal)
#endif

KNSBackend::KNSBackend(QObject *parent, const QString &iconName, const QString &knsrc)
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

    const KConfig conf(m_name, KConfig::SimpleConfig);
    if (!conf.hasGroup("KNewStuff3")) {
        markInvalid(QStringLiteral("Config group not found! Check your KNS3 installation."));
        return;
    }

    m_categories = QStringList{fileName};

    const KConfigGroup group = conf.group("KNewStuff3");
    m_extends = group.readEntry("Extends", QStringList());

    setFetching(true);

    // This ensures we have something to track when checking after the initialization timeout
    connect(this, &KNSBackend::initialized, this, [this]() {
        m_initialized = true;
    });
    // If we have not initialized in 60 seconds, consider this KNS backend invalid
    QTimer::singleShot(60000, this, [this]() {
        if (!m_initialized) {
            markInvalid(i18n("Backend %1 took too long to initialize", m_displayName));
            setResponsePending(false);
        }
    });

    const CategoryFilter filter = {CategoryFilter::CategoryNameFilter, fileName};
    const QSet<QString> backendName = {name()};
    m_displayName = group.readEntry("Name", QString());
    if (m_displayName.isEmpty()) {
        m_displayName = fileName.mid(0, fileName.indexOf(QLatin1Char('.')));
        m_displayName[0] = m_displayName[0].toUpper();
    }
    m_hasApplications = group.readEntry<bool>("X-Discover-HasApplications", false);

    const QStringList cats = group.readEntry<QStringList>("Categories", QStringList{});
    QVector<Category *> categories;
    if (cats.count() > 1) {
        m_categories += cats;
        for (const auto &cat : cats) {
            if (m_hasApplications)
                categories << new Category(cat, QStringLiteral("applications-other"), {CategoryFilter::CategoryNameFilter, cat}, backendName, {}, true);
            else
                categories << new Category(cat, QStringLiteral("plasma"), {CategoryFilter::CategoryNameFilter, cat}, backendName, {}, true);
        }
    }

    QVector<Category *> topCategories{categories};
    for (const auto &cat : std::as_const(categories)) {
        const QString catName = cat->name().append(QLatin1Char('/'));
        for (const auto &potentialSubCat : std::as_const(categories)) {
            if (potentialSubCat->name().startsWith(catName)) {
                cat->addSubcategory(potentialSubCat);
                topCategories.removeOne(potentialSubCat);
            }
        }
    }

    m_engine = new KNSCore::Engine(this);
    connect(m_engine, &KNSCore::Engine::signalErrorCode, this, &KNSBackend::slotErrorCode);
    connect(m_engine, &KNSCore::Engine::signalEntryEvent, this, &KNSBackend::slotEntryEvent);
    connect(m_engine, &KNSCore::Engine::signalEntriesLoaded, this, &KNSBackend::receivedEntries, Qt::QueuedConnection);
    connect(m_engine, &KNSCore::Engine::signalProvidersLoaded, this, &KNSBackend::fetchInstalled);
    connect(m_engine, &KNSCore::Engine::signalUpdateableEntriesLoaded, this, [this] {
        if (m_responsePending) {
            setResponsePending(false);
        }
    });
    connect(m_engine, &KNSCore::Engine::signalCategoriesMetadataLoded, this, [categories](const QList<KNSCore::Provider::CategoryMetadata> &categoryMetadatas) {
        for (const KNSCore::Provider::CategoryMetadata &category : categoryMetadatas) {
            for (Category *cat : std::as_const(categories)) {
                if (cat->matchesCategoryName(category.name)) {
                    cat->setName(category.displayName);
                    break;
                }
            }
        }
    });
    m_engine->setPageSize(ENGINE_PAGE_SIZE);
    m_engine->init(m_name);

    if (m_hasApplications) {
        auto actualCategory = new Category(m_displayName, QStringLiteral("applications-other"), filter, backendName, topCategories, false);
        auto applicationCategory = new Category(i18n("Applications"), //
                                                QStringLiteral("applications-internet"),
                                                filter,
                                                backendName,
                                                {actualCategory},
                                                false);
        const QVector<CategoryFilter> filters = {{CategoryFilter::CategoryNameFilter, QLatin1String("Application")}, filter};
        applicationCategory->setFilter({CategoryFilter::AndFilter, filters});
        m_categories.append(applicationCategory->name());
        m_rootCategories = {applicationCategory};
        // Make sure we filter out any apps which won't run on the current system architecture
        QStringList tagFilter = m_engine->tagFilter();
        if (QSysInfo::currentCpuArchitecture() == QLatin1String("arm")) {
            tagFilter << QLatin1String("application##architecture==armhf");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("arm64")) {
            tagFilter << QLatin1String("application##architecture==arm64");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("i386")) {
            tagFilter << QLatin1String("application##architecture==x86");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("ia64")) {
            tagFilter << QLatin1String("application##architecture==x86-64");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("x86_64")) {
            tagFilter << QLatin1String("application##architecture==x86");
            tagFilter << QLatin1String("application##architecture==x86-64");
        }
        m_engine->setTagFilter(tagFilter);
    } else {
        static const QSet<QString> knsrcPlasma = {
            QStringLiteral("aurorae.knsrc"),       QStringLiteral("icons.knsrc"),
            QStringLiteral("kfontinst.knsrc"),     QStringLiteral("lookandfeel.knsrc"),
            QStringLiteral("plasma-themes.knsrc"), QStringLiteral("plasmoids.knsrc"),
            QStringLiteral("wallpaper.knsrc"),     QStringLiteral("wallpaper-mobile.knsrc"),
            QStringLiteral("xcursor.knsrc"),

            QStringLiteral("cgcgtk3.knsrc"),       QStringLiteral("cgcicon.knsrc"),
            QStringLiteral("cgctheme.knsrc"), // GTK integration
            QStringLiteral("kwinswitcher.knsrc"),  QStringLiteral("kwineffect.knsrc"),
            QStringLiteral("kwinscripts.knsrc"), // KWin
            QStringLiteral("comic.knsrc"),         QStringLiteral("colorschemes.knsrc"),
            QStringLiteral("emoticons.knsrc"),     QStringLiteral("plymouth.knsrc"),
            QStringLiteral("sddmtheme.knsrc"),     QStringLiteral("wallpaperplugin.knsrc"),
            QStringLiteral("ksplash.knsrc"),       QStringLiteral("window-decorations.knsrc"),
        };
        const auto iconName = knsrcPlasma.contains(fileName) ? QStringLiteral("plasma") : QStringLiteral("applications-other");
        auto actualCategory = new Category(m_displayName, iconName, filter, backendName, categories, true);
        actualCategory->setParent(this);

        const auto topLevelName = knsrcPlasma.contains(fileName) ? i18n("Plasma Addons") : i18n("Application Addons");
        auto addonsCategory = new Category(topLevelName, iconName, filter, backendName, {actualCategory}, true);
        m_rootCategories = {addonsCategory};
    }

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &KNSBackend::updatesCountChanged);
}

KNSBackend::~KNSBackend()
{
    qDeleteAll(m_rootCategories);
}

void KNSBackend::markInvalid(const QString &message)
{
    m_rootCategories.clear();
    qWarning() << "invalid kns backend!" << m_name << "because:" << message;
    m_isValid = false;
    setFetching(false);
    Q_EMIT initialized();
}

void KNSBackend::setResponsePending(bool pending)
{
    Q_ASSERT(m_responsePending != pending);
    m_responsePending = pending;
    if (pending) {
        Q_EMIT startingSearch();
    } else {
        Q_EMIT availableForQueries();
        setFetching(false);
        m_onePage = false;
    }
}

void KNSBackend::fetchInstalled()
{
    auto search = new OneTimeAction(
        666,
        [this]() {
            // First we ensure we've got data loaded on what we've got installed already
            if (m_responsePending) {
                // Slot already taken, will need to wait again
                return false;
            }
            m_onePage = true;
            setResponsePending(true);
            m_engine->checkForInstalled();
            // And then we check for updates - we could do only one, if all we cared about was updates,
            // but to have both a useful initial list, /and/ information on updates, we want to get both.
            // The reason we are not doing a checkUpdates() overload for this is that the caching for this
            // information is done by KNSEngine, and we want to actually load it every time we initialize.
            auto updateChecker = new OneTimeAction(
                666,
                [this] {
                    // No need to check for updates if there's no resources
                    if (m_resourcesByName.isEmpty()) {
                        return true;
                    }

                    if (m_responsePending) {
                        // Slot already taken, will need to wait again
                        return false;
                    }

                    m_onePage = true;
                    setResponsePending(true);
                    m_engine->checkForUpdates();
                    return true;
                },
                this);
            connect(this, &KNSBackend::availableForQueries, updateChecker, &OneTimeAction::trigger, Qt::QueuedConnection);
            return true;
        },
        this);

    if (m_responsePending) {
        connect(this, &KNSBackend::availableForQueries, search, &OneTimeAction::trigger, Qt::QueuedConnection);
    } else {
        search->trigger();
    }
}

void KNSBackend::checkForUpdates()
{
    // Since we load the updates during initialization already, don't overburden
    // the machine with multiple of these, because that would just be silly.
    if (m_initialized) {
        auto updateChecker = new OneTimeAction(
            666,
            [this] {
                if (m_responsePending) {
                    // Slot already taken, will need to wait again
                    return false;
                }

                m_onePage = true;
                setResponsePending(true);
                m_engine->checkForUpdates();
                return true;
            },
            this);

        if (m_responsePending) {
            connect(this, &KNSBackend::availableForQueries, updateChecker, &OneTimeAction::trigger, Qt::QueuedConnection);
        } else {
            updateChecker->trigger();
        }
    }
}

void KNSBackend::setFetching(bool f)
{
    if (m_fetching != f) {
        m_fetching = f;
        Q_EMIT fetchingChanged();

        if (!m_fetching) {
            Q_EMIT initialized();
        }
    }
}

bool KNSBackend::isValid() const
{
    return m_isValid;
}

KNSResource *KNSBackend::resourceForEntry(const KNSCore::EntryInternal &entry)
{
    KNSResource *r = static_cast<KNSResource *>(m_resourcesByName.value(entry.uniqueId()));
    if (!r) {
        QStringList categories{name(), m_rootCategories.first()->name()};
        const auto cats = m_engine->categoriesMetadata();
        const int catIndex = kIndexOf(cats, [&entry](const KNSCore::Provider::CategoryMetadata &cat) {
            return entry.category() == cat.id;
        });
        if (catIndex > -1) {
            categories << cats.at(catIndex).name;
        }
        if (m_hasApplications) {
            categories << QLatin1String("Application");
        }
        r = new KNSResource(entry, categories, this);
        m_resourcesByName.insert(entry.uniqueId(), r);
    } else {
        r->setEntry(entry);
    }
    return r;
}

void KNSBackend::receivedEntries(const KNSCore::EntryInternal::List &entries)
{
    if (!m_isValid)
        return;

    const auto filtered = kFilter<KNSCore::EntryInternal::List>(entries, [](const KNSCore::EntryInternal &entry) {
        return entry.isValid();
    });
    const auto resources = kTransform<QVector<AbstractResource *>>(filtered, [this](const KNSCore::EntryInternal &entry) {
        return resourceForEntry(entry);
    });

    if (!resources.isEmpty()) {
        Q_EMIT receivedResources(resources);
    }

    setResponsePending(false);
    if (m_onePage || resources.count() < ENGINE_PAGE_SIZE) {
        Q_EMIT searchFinished();
    }
}

void KNSBackend::fetchMore()
{
    if (m_responsePending)
        return;

    // We _have_ to set this first. If we do not, we may run into a situation where the
    // data request will conclude immediately, causing m_responsePending to remain true
    // for perpetuity as the slots will be called before the function returns.
    setResponsePending(true);
    m_engine->requestMoreData();
}

void KNSBackend::statusChanged(const KNSCore::EntryInternal &entry)
{
    resourceForEntry(entry);
}

void KNSBackend::slotErrorCode(const KNSCore::ErrorCode &errorCode, const QString &message, const QVariant &metadata)
{
    QString error = message;
    qWarning() << "KNS error in" << m_displayName << ":" << errorCode << message << metadata;
    bool invalidFile = false;
    switch (errorCode) {
    case KNSCore::ErrorCode::UnknownError:
        // This is not supposed to be hit, of course, but any error coming to this point should be non-critical and safely ignored.
        break;
    case KNSCore::ErrorCode::NetworkError:
        // If we have a network error, we need to tell the user about it. This is almost always fatal, so mark invalid and tell the user.
        error = i18n("Network error in backend %1: %2", m_displayName, metadata.toInt());
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::OcsError:
        if (metadata.toInt() == 200) {
            // Too many requests, try again in a couple of minutes - perhaps we can simply postpone it automatically, and give a message?
            error = i18n("Too many requests sent to the server for backend %1. Please try again in a few minutes.", m_displayName);
        } else {
            // Unknown API error, usually something critical, mark as invalid and cry a lot
            error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
            markInvalid(error);
            invalidFile = true;
        }
        break;
    case KNSCore::ErrorCode::ConfigFileError:
        error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::ProviderError:
        error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::InstallationError: {
        KNSResource *r = static_cast<KNSResource *>(m_resourcesByName.value(metadata.toString()));
        if (r) {
            // If the following is true, then we can safely assume that the entry was
            // attempted updated, but the update was aborted.
            // Specifically, we can also likely expect that the update failed because
            // KNSCore::Engine was unable to deduce which payload to use (which will
            // happen when an entry has more than one payload, and none of those match
            // the name of the originally downloaded file).
            // We cannot complete this in Discover (as we've no way to forward that
            // query to the user) but we can give them an idea of how to deal with the
            // situation some other way.
            // TODO: Once Discover has a way to forward queries to the user from transactions, this likely will no longer be needed
            if (r->entry().status() == KNS3::Entry::Updateable) {
                error = i18n(
                    "Unable to complete the update of %1. You can try and perform this action through the Get Hot New Stuff dialog, which grants tighter "
                    "control. The reported error was:\n%2",
                    r->name(),
                    message);
            }
        }
        break;
    }
    case KNSCore::ErrorCode::ImageError:
        // Image fetching errors are not critical as such, but may lead to weird layout issues, might want handling...
        error = i18n("Could not fetch screenshot for the entry %1 in backend %2", metadata.toList().at(0).toString(), m_displayName);
        break;
    default:
        // Having handled all current error values, we should by all rights never arrive here, but for good order and future safety...
        error = i18n("Unhandled error in %1 backend. Contact your distributor.", m_displayName);
        break;
    }
    if (m_responsePending) {
        setResponsePending(false);
    }
    qWarning() << "kns error" << objectName() << error;
    if (!invalidFile)
        Q_EMIT passiveMessage(i18n("%1: %2", name(), error));
}

void KNSBackend::slotEntryEvent(const KNSCore::EntryInternal &entry, KNSCore::EntryInternal::EntryEvent event)
{
    switch (event) {
    case KNSCore::EntryInternal::StatusChangedEvent:
        statusChanged(entry);
        break;
    case KNSCore::EntryInternal::DetailsLoadedEvent:
        detailsLoaded(entry);
        break;
    case KNSCore::EntryInternal::AdoptedEvent:
    case KNSCore::EntryInternal::UnknownEvent:
    default:
        break;
    }
}

Transaction *KNSBackend::removeApplication(AbstractResource *app)
{
    auto res = qobject_cast<KNSResource *>(app);
    return new KNSTransaction(this, res, Transaction::RemoveRole);
}

Transaction *KNSBackend::installApplication(AbstractResource *app)
{
    auto res = qobject_cast<KNSResource *>(app);
    return new KNSTransaction(this, res, Transaction::InstallRole);
}

Transaction *KNSBackend::installApplication(AbstractResource *app, const AddonList & /*addons*/)
{
    return installApplication(app);
}

int KNSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractReviewsBackend *KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

static ResultsStream *voidStream()
{
    return new ResultsStream(QStringLiteral("KNS-void"), {});
}

ResultsStream *KNSBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!m_isValid || (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() != QLatin1String("kns")) || !filter.mimetype.isEmpty())
        return voidStream();

    if (filter.resourceUrl.scheme() == QLatin1String("kns")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (filter.state >= AbstractResource::Installed) {
        auto stream = new ResultsStream(QStringLiteral("KNS-installed"));

        const auto start = [this, stream, filter]() {
            if (m_isValid) {
                auto filterFunction = [&filter](AbstractResource *r) {
                    return r->state() >= filter.state
                        && (r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive));
                };
                const auto ret = kFilter<QVector<AbstractResource *>>(m_resourcesByName, filterFunction);

                if (!ret.isEmpty())
                    Q_EMIT stream->resourcesFound(ret);
            }
            stream->finish();
        };
        if (isFetching()) {
            connect(this, &KNSBackend::initialized, stream, start);
        } else {
            QTimer::singleShot(0, stream, start);
        }

        return stream;
    } else if ((!filter.category && !filter.search.isEmpty()) // Accept global searches
                                                              // If there /is/ a category, make sure we actually are one of those requested before searching
               || (filter.category && kContains(m_categories, [&filter](const QString &cat) {
                       return filter.category->matchesCategoryName(cat);
                   }))) {
        auto r = new ResultsStream(QLatin1String("KNS-search-") + name());
        searchStream(r, filter.search);
        return r;
    }
    return voidStream();
}

void KNSBackend::searchStream(ResultsStream *stream, const QString &searchText)
{
    Q_EMIT startingSearch();

    stream->setProperty("alreadyStarted", false);
    auto start = [this, stream, searchText]() {
        Q_ASSERT(!isFetching());
        if (!m_isValid) {
            qWarning() << "querying an invalid backend";
            stream->finish();
            return;
        }

        if (m_responsePending || stream->property("alreadyStarted").toBool()) {
            return;
        }
        stream->setProperty("alreadyStarted", true);
        setResponsePending(true);

        // No need to explicitly launch a search, setting the search term already does that for us
        m_engine->setSearchTerm(searchText);
        m_onePage = false;

        connect(stream, &ResultsStream::fetchMore, this, &KNSBackend::fetchMore);
        connect(this, &KNSBackend::receivedResources, stream, &ResultsStream::resourcesFound);
        connect(this, &KNSBackend::searchFinished, stream, &ResultsStream::finish);
        connect(this, &KNSBackend::startingSearch, stream, &ResultsStream::finish);
    };
    if (m_responsePending) {
        connect(this, &KNSBackend::availableForQueries, stream, start, Qt::QueuedConnection);
    } else if (isFetching()) {
        connect(this, &KNSBackend::initialized, stream, start, Qt::QueuedConnection);
        connect(this, &KNSBackend::availableForQueries, stream, start, Qt::QueuedConnection);
    } else {
        QTimer::singleShot(0, stream, start);
    }
}

ResultsStream *KNSBackend::findResourceByPackageName(const QUrl &search)
{
    if (search.scheme() != QLatin1String("kns") || search.host() != name())
        return voidStream();

    const auto pathParts = search.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.size() != 2) {
        Q_EMIT passiveMessage(i18n("Wrong KNewStuff URI: %1", search.toString()));
        return voidStream();
    }
    const auto providerid = pathParts.at(0);
    const auto entryid = pathParts.at(1);

    auto stream = new ResultsStream(QLatin1String("KNS-byname-") + entryid);
    auto start = [this, entryid, stream, providerid]() {
        if (m_responsePending) {
            // Slot already taken, will need to wait again
            return;
        }
        setResponsePending(true);
        m_engine->fetchEntryById(entryid);
        m_onePage = true;

        connect(m_engine, &KNSCore::Engine::signalErrorCode, stream, &ResultsStream::finish);
        connect(m_engine,
                &KNSCore::Engine::signalEntryEvent,
                stream,
                [this, stream, entryid, providerid](const KNSCore::EntryInternal &entry, KNSCore::EntryInternal::EntryEvent event) {
                    switch (event) {
                    case KNSCore::EntryInternal::StatusChangedEvent:
                        if (entry.uniqueId() == entryid && providerid == QUrl(entry.providerId()).host()) {
                            Q_EMIT stream->resourcesFound({resourceForEntry(entry)});
                        } else
                            qWarning() << "found invalid" << entryid << entry.uniqueId() << providerid << QUrl(entry.providerId()).host();
                        QTimer::singleShot(0, this, [this] {
                            setResponsePending(false);
                        });
                        stream->finish();
                        break;
                    case KNSCore::EntryInternal::DetailsLoadedEvent:
                    case KNSCore::EntryInternal::AdoptedEvent:
                    case KNSCore::EntryInternal::UnknownEvent:
                    default:
                        break;
                    }
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

AbstractBackendUpdater *KNSBackend::backendUpdater() const
{
    return m_updater;
}

QString KNSBackend::displayName() const
{
    return QStringLiteral("KNewStuff");
}

void KNSBackend::detailsLoaded(const KNSCore::EntryInternal &entry)
{
    auto res = resourceForEntry(entry);
    Q_EMIT res->longDescriptionChanged();
}

#include "KNSBackend.moc"
