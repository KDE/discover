/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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

#include "PackageKitResource.h"
#include "PackageKitBackend.h"
#include "PackageKitMessages.h"
#include <KLocalizedString>
#include <PackageKit/Details>
#include <PackageKit/Daemon>
#include <QJsonArray>
#include <QDebug>

#if defined(WITH_MARKDOWN)
extern "C" {
#include <mkdio.h>
}
#endif

const QStringList PackageKitResource::m_objects({ QStringLiteral("qrc:/qml/DependenciesButton.qml") });

PackageKitResource::PackageKitResource(QString packageName, QString summary, PackageKitBackend* parent)
    : AbstractResource(parent)
    , m_summary(std::move(summary))
    , m_name(std::move(packageName))
{
    setObjectName(m_name);

    connect(this, &PackageKitResource::dependenciesFound, this, [this](const QJsonObject& obj) { setDependenciesCount(obj.size()); });
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
    return { m_name };
}

QString PackageKitResource::availablePackageId() const
{
    //First we check if it's upgradeable and use this version to display
    const QString pkgid = backend()->upgradeablePackageId(this);
    if (!pkgid.isEmpty())
        return pkgid;

    QMap<PackageKit::Transaction::Info, QStringList>::const_iterator it = m_packages.constFind(PackageKit::Transaction::InfoAvailable);
    if (it != m_packages.constEnd())
        return it->last();
    return installedPackageId();
}

QString PackageKitResource::installedPackageId() const
{
    const auto installed = m_packages[PackageKit::Transaction::InfoInstalled];
    return installed.isEmpty() ? QString() : installed.last();
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

QString PackageKitResource::license()
{
    fetchDetails();
    return m_details.license().isEmpty() ? i18n("Unknown") : m_details.license();
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

int PackageKitResource::size()
{
    fetchDetails();
    return m_details.size();
}

QString PackageKitResource::origin() const
{
    auto pkgid = availablePackageId();
    return PackageKit::Daemon::packageData(pkgid);
}

QString PackageKitResource::section()
{
    return QString();
}

AbstractResource::State PackageKitResource::state()
{
    if (backend()->isPackageNameUpgradeable(this))
        return Upgradeable;
    else if(m_packages.contains(PackageKit::Transaction::InfoInstalled))
        return Installed;
    else if(m_packages.contains(PackageKit::Transaction::InfoAvailable))
        return None;
    else
        return Broken;
}

void PackageKitResource::addPackageId(PackageKit::Transaction::Info info, const QString &packageId, bool arch)
{
    auto oldState = state();
    if (arch)
        m_packages[info].append(packageId);
    else
        m_packages[info].prepend(packageId);

    if (oldState != state())
        emit stateChanged();
}

QStringList PackageKitResource::categories()
{
    return { QStringLiteral("Unknown") };
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
    m_details.insert(QStringLiteral("fetching"), true);//we add an entry so it's not re-fetched.

    backend()->fetchDetails(pkgid);
}

void PackageKitResource::failedFetchingDetails(PackageKit::Transaction::Error error, const QString& msg)
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

void PackageKitResource::setDetails(const PackageKit::Details & details)
{
    const bool ourDetails = details.packageId() == availablePackageId();
    if (!ourDetails)
        return;

    if (m_details != details) {
        m_details = details;
        emit stateChanged();

        if (!backend()->isFetching())
            Q_EMIT backend()->resourcesChanged(this, {"size", "homepage", "license"});
    }
}

void PackageKitResource::fetchChangelog()
{
}

void PackageKitResource::fetchUpdateDetails()
{
    const auto pkgid = availablePackageId();
    if (pkgid.isEmpty()) {
        connect(this, &PackageKitResource::stateChanged, this, &PackageKitResource::fetchUpdateDetails);
        return;
    }
    PackageKit::Transaction* t = PackageKit::Daemon::getUpdateDetail(availablePackageId());
    connect(t, &PackageKit::Transaction::updateDetail, this, &PackageKitResource::updateDetail);
    connect(t, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error err, const QString & error) { qWarning() << "error fetching updates:" << err << error; emit changelogFetched(QString()); });
}

static void addIfNotEmpty(const QString& title, const QString& content, QString& where)
{
    if (!content.isEmpty())
        where += QStringLiteral("<p><b>") + title + QStringLiteral("</b>&nbsp;") + QString(content).replace(QStringLiteral("\n"), QStringLiteral("<br />")) + QStringLiteral("</p>");
}

QString PackageKitResource::joinPackages(const QStringList& pkgids, const QString &_sep, const QString &shadowPackage)
{
    QStringList ret;
    foreach(const QString& pkgid, pkgids) {
        const auto pkgname = PackageKit::Daemon::packageName(pkgid);
        if (pkgname == shadowPackage)
            ret += PackageKit::Daemon::packageVersion(pkgid);
        else
            ret += i18nc("package-name (version)", "%1 (%2)", pkgname, PackageKit::Daemon::packageVersion(pkgid));
    }
    const QString sep = _sep.isEmpty() ? i18nc("comma separating package names", ", ") : _sep;
    return ret.join(sep);
}

static QStringList urlToLinks(const QStringList& urls)
{
    QStringList ret;
    foreach(const QString& in, urls)
        ret += QStringLiteral("<a href='%1'>%1</a>").arg(in);
    return ret;
}

void PackageKitResource::updateDetail(const QString& packageID, const QStringList& updates, const QStringList& obsoletes, const QStringList& vendorUrls,
                                      const QStringList& /*bugzillaUrls*/, const QStringList& /*cveUrls*/, PackageKit::Transaction::Restart restart, const QString &_updateText,
                                      const QString& /*changelog*/, PackageKit::Transaction::UpdateState state, const QDateTime& /*issued*/, const QDateTime& /*updated*/)
{
#if defined(WITH_MARKDOWN)
    const char* xx = _updateText.toUtf8().constData();
    MMIOT *markdownHandle = mkd_string(xx, _updateText.size(), 0);

    QString updateText;
    if ( !mkd_compile( markdownHandle, MKD_FENCEDCODE | MKD_GITHUBTAGS | MKD_AUTOLINK ) ) {
        updateText = _updateText;
    } else {
        char *htmlDocument;
        const int size = mkd_document( markdownHandle, &htmlDocument );

        updateText = QString::fromUtf8( htmlDocument, size );
    }
    mkd_cleanup( markdownHandle );

#else
    const auto& updateText = _updateText;
#endif

    const auto name = PackageKit::Daemon::packageName(packageID);

    QString info;
    addIfNotEmpty(i18n("Current Version:"), joinPackages(updates, {}, name), info);
    addIfNotEmpty(i18n("Obsoletes:"), joinPackages(obsoletes, {}, name), info);
    addIfNotEmpty(i18n("New Version:"), updateText, info);
    addIfNotEmpty(i18n("Update State:"), PackageKitMessages::updateStateMessage(state), info);
    addIfNotEmpty(i18n("Restart:"), PackageKitMessages::restartMessage(restart), info);

    if (!vendorUrls.isEmpty())
        addIfNotEmpty(i18n("Vendor:"), urlToLinks(vendorUrls).join(QStringLiteral(", ")), info);

    emit changelogFetched(changelog() + info);
}

PackageKitBackend* PackageKitResource::backend() const
{
    return qobject_cast<PackageKitBackend*>(parent());
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
    return QStringLiteral("package-available");
}

void PackageKitResource::fetchDependencies()
{
    const auto id = isInstalled() ? installedPackageId() : availablePackageId();
    if (id.isEmpty())
        return;
    m_dependenciesCount = 0;

    QSharedPointer<QJsonObject> packageDependencies(new QJsonObject);

    auto trans = PackageKit::Daemon::dependsOn(id);
    connect(trans, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error, const QString& message) { qWarning() << "Transaction error: " << message << sender(); });
    connect(trans, &PackageKit::Transaction::package, this, [packageDependencies](PackageKit::Transaction::Info /*info*/, const QString &packageID, const QString &summary) {
        (*packageDependencies)[PackageKit::Daemon::packageName(packageID)] = summary ;
    });
    connect(trans, &PackageKit::Transaction::finished, this, [this, packageDependencies](PackageKit::Transaction::Exit /*status*/) {
        Q_EMIT dependenciesFound(*packageDependencies);
    });
}
