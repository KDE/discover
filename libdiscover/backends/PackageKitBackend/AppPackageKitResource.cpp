/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AppPackageKitResource.h"
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/release.h>
#include <appstream/AppStreamUtils.h>
#include <PackageKit/Daemon>
#include <KLocalizedString>
#include <KToolInvocation>
#include <QIcon>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include "config-paths.h"
#include "utils.h"

AppPackageKitResource::AppPackageKitResource(const AppStream::Component& data, const QString &packageName, PackageKitBackend* parent)
    : PackageKitResource(packageName, QString(), parent)
    , m_appdata(data)
{
    Q_ASSERT(data.isValid());
}

QString AppPackageKitResource::name() const
{
    QString ret;
    if (!m_appdata.extends().isEmpty()) {
        auto components = backend()->componentsById(m_appdata.extends().constFirst());

        if (components.isEmpty())
            qWarning() << "couldn't find" << m_appdata.extends() << "which is supposedly extended by" << m_appdata.id();
        else
            ret = components.constFirst().name() + QLatin1String(" - ") + m_appdata.name();
    }

    if (ret.isEmpty())
        ret = m_appdata.name() + QLatin1String("<font size=\"-2\">  &nbsp;&nbsp; ") + availableVersion() + QLatin1String("</font>");
    return ret;
}

QString AppPackageKitResource::longDescription()
{
    const auto desc = m_appdata.description();
    if (!desc.isEmpty())
        return desc;

    return PackageKitResource::longDescription();
}

static QIcon componentIcon(const AppStream::Component &comp)
{
    QIcon ret;
    foreach(const AppStream::Icon &icon, comp.icons()) {
        QStringList stock;
        switch(icon.kind()) {
            case AppStream::Icon::KindLocal:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            case AppStream::Icon::KindCached:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            case AppStream::Icon::KindStock: {
                const auto ret = QIcon::fromTheme(icon.name());
                if (!ret.isNull())
                    return ret;
                break;
            }
            default:
                break;
        }
    }
    if (ret.isNull()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    }
    return ret;
}

QVariant AppPackageKitResource::icon() const
{
    return componentIcon(m_appdata);
}

QJsonArray AppPackageKitResource::licenses()
{
    return m_appdata.projectLicense().isEmpty() ? PackageKitResource::licenses() : AppStreamUtils::licenses(m_appdata);
}

QStringList AppPackageKitResource::mimetypes() const
{
    return m_appdata.provided(AppStream::Provided::KindMimetype).items();
}

QStringList AppPackageKitResource::categories()
{
    auto cats = m_appdata.categories();
    if (m_appdata.kind() != AppStream::Component::KindAddon)
        cats.append(QStringLiteral("Application"));
    return cats;
}

QString AppPackageKitResource::comment()
{
    const auto summary = m_appdata.summary();
    if (!summary.isEmpty())
        return summary;

    return PackageKitResource::comment();
}

QString AppPackageKitResource::appstreamId() const
{
    return m_appdata.id();
}

QUrl AppPackageKitResource::homepage()
{
    return m_appdata.url(AppStream::Component::UrlKindHomepage);
}

QUrl AppPackageKitResource::helpURL()
{
    return m_appdata.url(AppStream::Component::UrlKindHelp);
}

QUrl AppPackageKitResource::bugURL()
{
    return m_appdata.url(AppStream::Component::UrlKindBugtracker);
}

QUrl AppPackageKitResource::donationURL()
{
    return m_appdata.url(AppStream::Component::UrlKindDonation);
}

AbstractResource::Type AppPackageKitResource::type() const
{
    static QString desktop = QString::fromUtf8(qgetenv("XDG_CURRENT_DESKTOP"));
    const auto desktops = m_appdata.compulsoryForDesktops();
    return m_appdata.kind() == AppStream::Component::KindAddon   ? Addon
           : (desktops.isEmpty() || !desktops.contains(desktop)) ? Application
                                                                 : Technical;
}

void AppPackageKitResource::fetchScreenshots()
{
    const auto sc = AppStreamUtils::fetchScreenshots(m_appdata);
    Q_EMIT screenshotsFetched(sc.first, sc.second);
}

QStringList AppPackageKitResource::allPackageNames() const
{
    auto ret = m_appdata.packageNames();
    if (ret.isEmpty()) {
        ret = QStringList{ PackageKit::Daemon::packageName(availablePackageId()) };
    }
    return ret;
}

QList<PackageState> AppPackageKitResource::addonsInformation()
{
    const auto res = kFilter<QVector<AppPackageKitResource*>>(backend()->extendedBy(m_appdata.id()), [this](AppPackageKitResource* r){ return r->allPackageNames() != allPackageNames(); });
    return kTransform<QList<PackageState>>(res,
        [](AppPackageKitResource* r) { return PackageState(r->appstreamId(), r->name(), r->comment(), r->isInstalled()); }
    );
}

QStringList AppPackageKitResource::extends() const
{
    return m_appdata.extends();
}

QString AppPackageKitResource::changelog() const
{
    return AppStreamUtils::changelogToHtml(m_appdata);
}


bool AppPackageKitResource::canExecute() const
{
    static QSet<QString> cannotExecute = { QStringLiteral("org.kde.development") };
    return !cannotExecute.contains(m_appdata.id());
}

void AppPackageKitResource::invokeApplication() const
{
    auto trans = PackageKit::Daemon::getFiles({installedPackageId()});
    connect(trans, &PackageKit::Transaction::files, this, [this](const QString &/*packageID*/, const QStringList &filenames) {
        const auto allServices = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, m_appdata.id());
        if (!allServices.isEmpty()) {
            const auto packageServices = kFilter<QStringList>(allServices, [filenames](const QString &file) { return filenames.contains(file); });
            QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {packageServices});
            return;
        } else {
            const QStringList exes = m_appdata.provided(AppStream::Provided::KindBinary).items();
            const auto packageExecutables = kFilter<QStringList>(allServices, [filenames](const QString &exe) { return filenames.contains(QLatin1Char('/') + exe); });
            if (!packageExecutables.isEmpty()) {
                QProcess::startDetached(exes.constFirst());
                return;
            } else {
                const auto locations = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
                const auto desktopFiles = kFilter<QStringList>(filenames, [locations](const QString &exe) {
                    for (const auto &location: locations) {
                        if (exe.startsWith(location))
                            return exe.contains(QLatin1String(".desktop"));
                    }
                    return false;
                });
                if (!desktopFiles.isEmpty()) {
                    QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), { desktopFiles });
                    return;
                }
            }
            Q_EMIT backend()->passiveMessage(i18n("Cannot launch %1", name()));
        }
    });
}

QDate AppPackageKitResource::releaseDate() const
{
    if (!m_appdata.releases().isEmpty()) {
        auto release = m_appdata.releases().constFirst();
        return release.timestamp().date();
    }

    return {};
}

QString AppPackageKitResource::author() const
{
    return m_appdata.developerName();
}

void AppPackageKitResource::fetchChangelog()
{
    emit changelogFetched(changelog());
}
