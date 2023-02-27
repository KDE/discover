/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitResource.h"
#include "PackageKitBackend.h"
#include "PackageKitMessages.h"
#include "appstream/AppStreamUtils.h"
#include "config-paths.h"
#include <AppStreamQt/spdx.h>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <KShell>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QJsonArray>
#include <QProcess>
#include <appstream/AppStreamIntegration.h>
#include <utils.h>

#if defined(WITH_MARKDOWN)
extern "C" {
#include <mkdio.h>
}
#endif

const QStringList PackageKitResource::m_objects({QStringLiteral("qrc:/qml/DependenciesButton.qml"), QStringLiteral("qrc:/qml/PackageKitPermissions.qml")});

PackageKitResource::PackageKitResource(QString packageName, QString summary, PackageKitBackend *parent)
    : AbstractResource(parent)
    , m_summary(std::move(summary))
    , m_name(std::move(packageName))
{
    setObjectName(m_name);
}

QString PackageKitResource::name() const
{
    return m_name;
}

QString PackageKitResource::packageName() const
{
    return m_name;
}

QStringList PackageKitResource::allPackageNames() const
{
    return {m_name};
}

QString PackageKitResource::availablePackageId() const
{
    // First we check if it's upgradeable and use this version to display
    const QSet<QString> pkgids = backend()->upgradeablePackageId(this);
    if (!pkgids.isEmpty())
        return *pkgids.constBegin();

    const auto it = m_packages.constFind(PackageKit::Transaction::InfoAvailable);
    if (it != m_packages.constEnd())
        return it->first();
    return installedPackageId();
}

QString PackageKitResource::installedPackageId() const
{
    const auto installed = m_packages[PackageKit::Transaction::InfoInstalled];
    return installed.isEmpty() ? QString() : installed.first();
}

QString PackageKitResource::comment()
{
    return m_summary;
}

QString PackageKitResource::longDescription()
{
    fetchDetails();
    return m_details.description();
}

QUrl PackageKitResource::homepage()
{
    fetchDetails();
    return QUrl(m_details.url());
}

QVariant PackageKitResource::icon() const
{
    return QStringLiteral("applications-other");
}

static QMap<QString, QString> s_translation = {
    {"AGPL", "AGPL-3.0"},
    {"AGPL3", "AGPL-3.0"},
    {"Artistic2.0", "Artistic-2.0"},
    {"Apache", "Apache-2.0"},
    {"APACHE", "Apache-2.0"},
    {"CCPL", "CC0-1.0"},
    {"GPL2", "GPL-2.0"},
    {"GPL3", "GPL-3.0"},
    {"FDL1.2", "GFDL-1.2-only"},
    {"FDL1.3", "GFDL-1.3-only"},
    {"LGPL", "LGPL-2.1"},
    {"LGPL3", "LGPL-3.0"},
    {"MPL", "MPL-1.1"},
    {"MPL2", "MPL-2.0"},
    {"PerlArtistic", "Artistic-1.0-Perl"},
    {"PHP", "PHP-3.01"},
    {"PSF", "Python-2.0"},
    {"RUBY", "Ruby"},
    {"ZPL", "ZPL-2.1"},
};

QJsonArray PackageKitResource::licenses()
{
    fetchDetails();

    if (!m_details.license().isEmpty()) {
        QString id = m_details.license();
        if (!AppStream::SPDX::isLicenseId(id)) {
            auto spdxId = AppStream::SPDX::asSpdxId(id);
            if (!spdxId.isEmpty()) {
                id = spdxId;
            }
        }

        if (!AppStream::SPDX::isLicenseId(id)) {
            id = s_translation.value(id, id);
        }
        return {AppStreamUtils::license(id)};
    }

    return {QJsonObject{{QStringLiteral("name"), {}}}};
}

QList<PackageState> PackageKitResource::addonsInformation()
{
    return QList<PackageState>();
}

QString PackageKitResource::availableVersion() const
{
    return PackageKit::Daemon::packageVersion(availablePackageId());
}

QString PackageKitResource::installedVersion() const
{
    return PackageKit::Daemon::packageVersion(installedPackageId());
}

quint64 PackageKitResource::size()
{
    fetchDetails();
    return m_details.size();
}

QString PackageKitResource::origin() const
{
    auto osRelease = AppStreamIntegration::global()->osRelease();

    if (PackageKit::Daemon::backendName() == QStringLiteral("apt")) {
        // Debian and its derivatives have a defined scheme for repository origins that we can parse, to
        // guess a better origin name.
        QString pkgid = availablePackageId();
        QString dataField = PackageKit::Daemon::packageData(pkgid);
        // The "data" field of a package-id may contain a modifier such as "auto:" or "manual:", so
        // we will need to strip that in case it exists to extract the actual origin.
        // The data field may look like "auto:debian-bookworm-main" or "google_llc-stable-main",
        // so we can set the OS name if we see it as origin prefix, and otherwise need to fall back
        // to the origin string.
        int i = dataField.indexOf(':');
        QString origin = i > 0? dataField.mid(i + 1) : dataField;
        if (origin.startsWith(osRelease->id().toLower() + '-')) {
            return osRelease->name();
        } else {
            return origin.isEmpty()? i18n("Unknown Source") : origin;
        }
    }

    // PackageKit doesn't give us enough information to be able to distinguish
    // between 3rd-party repos (which generally have human-readable names) and
    // 1st-party distro repos (which generally name nonsense jargon names) which
    // would allow us to substitute the distro name for only the nonsense repos;
    // see https://github.com/PackageKit/PackageKit/issues/607 and
    // https://bugs.kde.org/show_bug.cgi?id=465204.
    // So for now always show the distro name.
    return osRelease->name();
}

QString PackageKitResource::section()
{
    return QString();
}

AbstractResource::State PackageKitResource::state()
{
    if (backend()->isPackageNameUpgradeable(this))
        return Upgradeable;
    else if (m_packages.contains(PackageKit::Transaction::InfoInstalled))
        return Installed;
    else if (m_packages.contains(PackageKit::Transaction::InfoAvailable))
        return None;
    else
        return Broken;
}

void PackageKitResource::addPackageId(PackageKit::Transaction::Info info, const QString &packageId, bool arch)
{
    auto oldState = state();
    if (arch)
        m_packages[info].archPkgIds.append(packageId);
    else
        m_packages[info].nonarchPkgIds.append(packageId);

    if (oldState != state())
        Q_EMIT stateChanged();

    Q_EMIT versionsChanged();
}

QStringList PackageKitResource::categories()
{
    return {QStringLiteral("Unknown")};
}

AbstractResource::Type PackageKitResource::type() const
{
    return Technical;
}

void PackageKitResource::fetchDetails()
{
    const QString pkgid = availablePackageId();
    if (!m_details.isEmpty() || pkgid.isEmpty())
        return;
    m_details.insert(QStringLiteral("fetching"), true); // we add an entry so it's not re-fetched.

    backend()->fetchDetails(pkgid);
}

void PackageKitResource::failedFetchingDetails(PackageKit::Transaction::Error error, const QString &msg)
{
    qWarning() << "error fetching details" << error << msg;
}

void PackageKitResource::setDependenciesCount(int deps)
{
    if (deps != m_dependenciesCount) {
        m_dependenciesCount = deps;
        Q_EMIT sizeChanged();
    }
}

void PackageKitResource::setDetails(const PackageKit::Details &details)
{
    const bool ourDetails = details.packageId() == availablePackageId();
    if (!ourDetails)
        return;

    if (m_details != details) {
        const auto oldState = state();
        const auto oldSize = m_details.size();
        const auto oldLicense = m_details.license();
        const auto oldDescription = m_details.description();
        m_details = details;

        if (oldState != state())
            Q_EMIT stateChanged();

        if (!backend()->isFetching())
            Q_EMIT backend()->resourcesChanged(this, {"size", "homepage", "license"});

        if (oldSize != uint(size())) {
            Q_EMIT sizeChanged();
        }

        if (oldLicense != m_details.license()) {
            Q_EMIT licensesChanged();
        }

        if (oldDescription != m_details.description()) {
            Q_EMIT longDescriptionChanged();
        }
    }
}

void PackageKitResource::fetchChangelog()
{
}

void PackageKitResource::fetchUpdateDetails()
{
    const auto pkgid = availablePackageId();
    if (pkgid.isEmpty()) {
        auto a = new OneTimeAction(
            [this] {
                fetchUpdateDetails();
            },
            this);
        connect(this, &PackageKitResource::stateChanged, a, &OneTimeAction::trigger);
        return;
    }
    PackageKit::Transaction *t = PackageKit::Daemon::getUpdateDetail(availablePackageId());
    connect(t, &PackageKit::Transaction::updateDetail, this, &PackageKitResource::updateDetail);
    connect(t, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error err, const QString &error) {
        qWarning() << "error fetching updates:" << err << error;
        Q_EMIT changelogFetched(QString());
    });
}

static void addIfNotEmpty(const QString &title, const QString &content, QString &where)
{
    if (!content.isEmpty())
        where += QLatin1String("<p><b>") + title + QLatin1String("</b>&nbsp;") + QString(content).replace(QLatin1Char('\n'), QLatin1String("<br />"))
            + QLatin1String("</p>");
}

QString PackageKitResource::joinPackages(const QStringList &pkgids, const QString &_sep, const QString &shadowPackage)
{
    QStringList ret;
    for (const QString &pkgid : pkgids) {
        const auto pkgname = PackageKit::Daemon::packageName(pkgid);
        if (pkgname == shadowPackage)
            ret += PackageKit::Daemon::packageVersion(pkgid);
        else
            ret += i18nc("package-name (version)", "%1 (%2)", pkgname, PackageKit::Daemon::packageVersion(pkgid));
    }
    const QString sep = _sep.isEmpty() ? i18nc("comma separating package names", ", ") : _sep;
    return ret.join(sep);
}

static QStringList urlToLinks(const QStringList &urls)
{
    QStringList ret;
    for (const QString &in : urls)
        ret += QStringLiteral("<a href='%1'>%1</a>").arg(in);
    return ret;
}

void PackageKitResource::updateDetail(const QString &packageID,
                                      const QStringList & /*updates*/,
                                      const QStringList &obsoletes,
                                      const QStringList &vendorUrls,
                                      const QStringList & /*bugzillaUrls*/,
                                      const QStringList & /*cveUrls*/,
                                      PackageKit::Transaction::Restart restart,
                                      const QString &_updateText,
                                      const QString & /*changelog*/,
                                      PackageKit::Transaction::UpdateState state,
                                      const QDateTime & /*issued*/,
                                      const QDateTime & /*updated*/)
{
#if defined(WITH_MARKDOWN)
    const QByteArray xx = _updateText.toUtf8();
    MMIOT *markdownHandle = mkd_string(xx.constData(), _updateText.size(), 0);

    if (!mkd_compile(markdownHandle, MKD_FENCEDCODE | MKD_GITHUBTAGS | MKD_AUTOLINK)) {
        m_changelog = _updateText;
    } else {
        char *htmlDocument;
        const int size = mkd_document(markdownHandle, &htmlDocument);

        m_changelog = QString::fromUtf8(htmlDocument, size);
    }
    mkd_cleanup(markdownHandle);

#else
    m_changelog = _updateText;
#endif

    const auto name = PackageKit::Daemon::packageName(packageID);

    QString info;
    addIfNotEmpty(i18n("Obsoletes:"), joinPackages(obsoletes, {}, name), info);
    addIfNotEmpty(i18n("Release Notes:"), changelog(), info);
    addIfNotEmpty(i18n("Update State:"), PackageKitMessages::updateStateMessage(state), info);
    addIfNotEmpty(i18n("Restart:"), PackageKitMessages::restartMessage(restart), info);

    if (!vendorUrls.isEmpty())
        addIfNotEmpty(i18n("Vendor:"), urlToLinks(vendorUrls).join(QLatin1String(", ")), info);

    Q_EMIT changelogFetched(info);
}

PackageKitBackend *PackageKitResource::backend() const
{
    return qobject_cast<PackageKitBackend *>(parent());
}

QString PackageKitResource::sizeDescription()
{
    if (m_dependenciesCount < 0) {
        fetchDetails();
        fetchDependencies();
    }

    if (m_dependenciesCount <= 0)
        return AbstractResource::sizeDescription();
    else
        return i18np("%2 (plus %1 dependency)", "%2 (plus %1 dependencies)", m_dependenciesCount, AbstractResource::sizeDescription());
}

QString PackageKitResource::sourceIcon() const
{
    return QStringLiteral("package-x-generic");
}

void PackageKitResource::fetchDependencies()
{
    const auto id = isInstalled() ? installedPackageId() : availablePackageId();
    if (id.isEmpty())
        return;
    m_dependenciesCount = 0;

    auto packageDependencies = QSharedPointer<QJsonArray>::create();

    auto trans = PackageKit::Daemon::dependsOn(id);
    connect(trans, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error, const QString &message) {
        qWarning() << "Transaction error: " << message << sender();
    });
    connect(trans,
            &PackageKit::Transaction::package,
            this,
            [packageDependencies](PackageKit::Transaction::Info info, const QString &packageID, const QString &summary) {
                (*packageDependencies)
                    .append(QJsonObject{{QStringLiteral("packageName"), PackageKit::Daemon::packageName(packageID)},
                                        {QStringLiteral("packageInfo"), PackageKitMessages::info(info)},
                                        {QStringLiteral("packageDescription"), summary}});
            });
    connect(trans, &PackageKit::Transaction::finished, this, [this, packageDependencies](PackageKit::Transaction::Exit /*status*/) {
        std::sort(packageDependencies->begin(), packageDependencies->end(), [](QJsonValue a, QJsonValue b) {
            const auto objA = a.toObject(), objB = b.toObject();
            return objA[QLatin1String("packageInfo")].toString() < objB[QLatin1String("packageInfo")].toString()
                || (objA[QLatin1String("packageInfo")].toString() == objB[QLatin1String("packageInfo")].toString()
                    && objA[QLatin1String("packageName")].toString() < objB[QLatin1String("packageName")].toString());
        });

        Q_EMIT dependenciesFound(*packageDependencies);
        setDependenciesCount(packageDependencies->size());
    });
}

bool PackageKitResource::extendsItself() const
{
    const auto extendsResources = backend()->resourcesByPackageNames<QVector<AbstractResource *>>(extends());
    if (extendsResources.isEmpty())
        return false;

    const auto ourPackageNames = allPackageNames();
    for (auto r : extendsResources) {
        PackageKitResource *pkres = qobject_cast<PackageKitResource *>(r);
        if (pkres->allPackageNames() != ourPackageNames)
            return false;
    }
    return true;
}

void PackageKitResource::runService(KService::Ptr service) const
{
    auto *job = new KIO::ApplicationLauncherJob(service);
    connect(job, &KJob::finished, this, [this, service](KJob *job) {
        if (job->error()) {
            Q_EMIT backend()->passiveMessage(i18n("Failed to start '%1': %2", service->name(), job->errorString()));
        }
    });

    job->start();
}

bool PackageKitResource::isCritical() const
{
    return false;
}
