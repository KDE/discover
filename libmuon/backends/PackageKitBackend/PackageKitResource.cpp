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
#include <MuonDataSources.h>
#include <KLocalizedString>
#include <PackageKit/Details>
#include <PackageKit/Daemon>

PackageKitResource::PackageKitResource(const QString &packageName, const QString &summary, PackageKitBackend* parent)
    : AbstractResource(parent)
    , m_summary(summary)
    , m_name(packageName)
{
    setObjectName(m_name);
}

QString PackageKitResource::name()
{
    return m_name;
}

QString PackageKitResource::packageName() const
{
    return m_name;
}

QStringList PackageKitResource::allPackageNames() const
{
    return QStringList(m_name);
}

QString PackageKitResource::availablePackageId() const
{
    //First we check if it's upgradeable and use this version to display
    const QString pkgid = backend()->upgradeablePackageId(this);
    if (!pkgid.isEmpty())
        return pkgid;

    QMap<PackageKit::Transaction::Info, QStringList>::const_iterator it = m_packages.constFind(PackageKit::Transaction::InfoAvailable);
    if (it != m_packages.constEnd())
        return it->first();
    return installedPackageId();
}

QString PackageKitResource::installedPackageId() const
{
    return m_packages[PackageKit::Transaction::InfoInstalled].first();
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
    return m_details.url();
}

QString PackageKitResource::icon() const
{
    return QStringLiteral("applications-other");
}

QString PackageKitResource::license()
{
    fetchDetails();
    return m_details.license();
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

int PackageKitResource::downloadSize()
{
    fetchDetails();
    return m_details.size();
}

QString PackageKitResource::origin() const
{
    //TODO
    return "PackageKit";
}

QString PackageKitResource::section()
{
    return QString();
}

QUrl PackageKitResource::screenshotUrl()
{
    return QUrl(MuonDataSources::screenshotsSource().toString() + "/screenshot/" + packageName());
}

QUrl PackageKitResource::thumbnailUrl()
{
    return QUrl(MuonDataSources::screenshotsSource().toString() + "/thumbnail/" + packageName());
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

void PackageKitResource::resetPackageIds()
{
    m_packages.clear();
}

void PackageKitResource::addPackageId(PackageKit::Transaction::Info info, const QString &packageId, const QString &/*summary*/)
{
    m_packages[info].append(packageId);
    emit stateChanged();
}

QStringList PackageKitResource::categories()
{
    /*fetchDetails();
    QStringList categories;
    switch (m_group) {
        case PackageKit::Transaction::GroupUnknown:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupAccessibility:
            categories << "Accessibility";
            break;
        case PackageKit::Transaction::GroupAccessories:
            categories << "Utility";
            break;
        case PackageKit::Transaction::GroupAdminTools:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupCommunication:
            categories << "Chat";
            break;
        case PackageKit::Transaction::GroupDesktopGnome:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopKde:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopOther:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopXfce:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupEducation:
            categories << "Science";
            break;
        case PackageKit::Transaction::GroupFonts:
            categories << "Fonts";
            break;
        case PackageKit::Transaction::GroupGames:
            categories << "Games";
            break;
        case PackageKit::Transaction::GroupGraphics:
            categories << "Graphics";
            break;
        case PackageKit::Transaction::GroupInternet:
            categories << "Internet";
            break;
        case PackageKit::Transaction::GroupLegacy:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupLocalization:
            categories << "Localization";
            break;
        case PackageKit::Transaction::GroupMaps:
            categories << "Geography";
            break;
        case PackageKit::Transaction::GroupMultimedia:
            categories << "Multimedia";
            break;
        case PackageKit::Transaction::GroupNetwork:
            categories << "Network";
            break;
        case PackageKit::Transaction::GroupOffice:
            categories << "Office";
            break;
        case PackageKit::Transaction::GroupOther:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupPowerManagement:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupProgramming:
            categories << "Development";
            break;
        case PackageKit::Transaction::GroupPublishing:
            categories << "Publishing";
            break;
        case PackageKit::Transaction::GroupRepos:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupSecurity:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupServers:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupSystem:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupVirtualization:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupScience:
            categories << "Science";
            break;
        case PackageKit::Transaction::GroupDocumentation:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupElectronics:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupCollections:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupVendor:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupNewest:
            categories << "System";
            break;
    };
    return categories;*/
    return QStringList() << "Unknown";
    //NOTE: I commented the category fetching code, as it seems to get called even for non-technical items
    //when selecting a category, and receiving details for all packages takes about 20 mins in my VirtualBox and probably not much less on real systems
}

bool PackageKitResource::isTechnical() const
{
    return true;//!m_availablePackageId.startsWith("flash");
}

void PackageKitResource::fetchDetails()
{
    if (!m_details.isEmpty())
        return;
    m_details.insert("fetching", true);//we add an entry so it's not re-fetched.

    PackageKit::Transaction* t = PackageKit::Daemon::getDetails(availablePackageId());
    connect(t, SIGNAL(details(PackageKit::Details)), this, SLOT(setDetails(PackageKit::Details)));
    connect(t, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error, const QString& msg){ qWarning() << "error fetching details" << msg; });
}

void PackageKitResource::setDetails(const PackageKit::Details & details)
{
    if (!m_packages.value(PackageKit::Transaction::InfoAvailable).contains(details.packageId()))
        return;

    m_details = details;

    emit stateChanged();
}

void PackageKitResource::fetchChangelog()
{
    PackageKit::Transaction* t = PackageKit::Daemon::getUpdateDetail(availablePackageId());
    connect(t, &PackageKit::Transaction::updateDetail, this, &PackageKitResource::updateDetail);
    connect(t, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error err, const QString & error) {
        qWarning() << "error fetching updates:" << err << error;
        emit changelogFetched(QString());
    });
}

static void addIfNotEmpty(const QString& title, const QString& content, QString& where)
{
    if (!content.isEmpty())
        where += QStringLiteral("<p><b>") + title + QStringLiteral("</b>&nbsp;") + content + QStringLiteral("</p>");
}

static QString joinPackages(const QStringList& pkgids)
{
    QStringList ret;
    foreach(const QString& pkgid, pkgids) {
        ret += i18nc("package-name (version)", "%1 (%2)", PackageKit::Daemon::packageName(pkgid), PackageKit::Daemon::packageVersion(pkgid));
    }
    return ret.join(i18nc("comma separating package names", ", "));
}

static QStringList urlToLinks(const QStringList& urls)
{
    QStringList ret;
    foreach(const QString& in, urls)
        ret += QStringLiteral("<a href='%1'>%1</a>").arg(in);
    return ret;
}

void PackageKitResource::updateDetail(const QString& /*packageID*/, const QStringList& updates, const QStringList& obsoletes, const QStringList& vendorUrls,
                                      const QStringList& /*bugzillaUrls*/, const QStringList& /*cveUrls*/, PackageKit::Transaction::Restart restart, const QString& updateText,
                                      const QString& changelog, PackageKit::Transaction::UpdateState state, const QDateTime& /*issued*/, const QDateTime& /*updated*/)
{
    QString info;
    addIfNotEmpty(i18n("Reason:"), updateText, info);
    addIfNotEmpty(i18n("Obsoletes:"), joinPackages(obsoletes), info);
    addIfNotEmpty(i18n("Updates:"), joinPackages(updates), info);
    addIfNotEmpty(i18n("Change Log:"), changelog, info);
    addIfNotEmpty(i18n("Update State:"), PackageKitMessages::updateStateMessage(state), info);
    addIfNotEmpty(i18n("Restart:"), PackageKitMessages::restartMessage(restart), info);

    if (!vendorUrls.isEmpty())
        addIfNotEmpty(i18n("Vendor:"), urlToLinks(vendorUrls).join(QStringLiteral(", ")), info);

    emit changelogFetched(info);
}

PackageKitBackend* PackageKitResource::backend() const
{
    return qobject_cast<PackageKitBackend*>(parent());
}

