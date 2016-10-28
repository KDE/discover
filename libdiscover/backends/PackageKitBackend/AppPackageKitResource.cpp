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
#include <KLocalizedString>
#include <KToolInvocation>
#include <QIcon>
#include <QProcess>
#include <QDebug>

AppPackageKitResource::AppPackageKitResource(const AppStream::Component& data, const QString &packageName, PackageKitBackend* parent)
    : PackageKitResource(packageName, QString(), parent)
    , m_appdata(data)
{
}

QString AppPackageKitResource::name()
{
    return m_appdata.name();
}

QString AppPackageKitResource::longDescription()
{
    const auto desc = m_appdata.description();
    if (!desc.isEmpty())
        return desc;

    return PackageKitResource::longDescription();
}

QVariant AppPackageKitResource::icon() const
{
    QIcon ret;
    const auto icons = m_appdata.icons();
    if (icons.isEmpty()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else foreach(const AppStream::Icon &icon, icons) {
        switch(icon.kind()) {
            case AppStream::Icon::KindLocal:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            case AppStream::Icon::KindCached:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            default:
                break;
        }
    }

    return ret;
}

QString AppPackageKitResource::license()
{
    const auto license = m_appdata.projectLicense();
    return license.isEmpty() ? PackageKitResource::license() : license;
}

QStringList AppPackageKitResource::mimetypes() const
{
    return findProvides(AppStream::Provided::KindMimetype);
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

bool AppPackageKitResource::isTechnical() const
{
    return false;
}

QStringList AppPackageKitResource::executables() const
{
    return findProvides(AppStream::Provided::KindBinary);
}

void AppPackageKitResource::invokeApplication() const
{
    QStringList exes = executables();
    if(!exes.isEmpty())
        QProcess::startDetached(exes.first());
}

static QUrl imageOfKind(const QList<AppStream::Image>& images, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Image &i, images) {
        if (i.kind() == kind) {
            ret = i.url();
            break;
        }
    }
    return ret;
}

static QUrl screenshot(const AppStream::Component& comp, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Screenshot &s, comp.screenshots()) {
        ret = imageOfKind(s.images(), kind);
        if (s.isDefault() && !ret.isEmpty())
            break;
    }
    return ret;
}

QUrl AppPackageKitResource::screenshotUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindSource);

}

QUrl AppPackageKitResource::thumbnailUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindThumbnail);
}

void AppPackageKitResource::fetchScreenshots()
{
    QList<QUrl> thumbnails, screenshots;

    Q_FOREACH (const AppStream::Screenshot &s, m_appdata.screenshots()) {
        const QUrl thumbnail = imageOfKind(s.images(), AppStream::Image::KindThumbnail);
        const QUrl plain = imageOfKind(s.images(), AppStream::Image::KindSource);
        if (plain.isEmpty())
            qWarning() << "invalid screenshot for" << name();

        screenshots << plain;
        thumbnails << (thumbnail.isEmpty() ? plain : thumbnail);
    }

    Q_EMIT screenshotsFetched(thumbnails, screenshots);
}

bool AppPackageKitResource::canExecute() const
{
    return !executables().isEmpty();
}

QStringList AppPackageKitResource::findProvides(AppStream::Provided::Kind kind) const
{
    QStringList ret;
    Q_FOREACH (AppStream::Provided p, m_appdata.provided())
        if (p.kind() == kind)
            ret += p.items();
    return ret;
}

QStringList AppPackageKitResource::allPackageNames() const
{
    return m_appdata.packageNames();
}

QList<PackageState> AppPackageKitResource::addonsInformation()
{
    const PackageKitBackend* p = static_cast<PackageKitBackend*>(parent());
    const QVector<AppPackageKitResource*> res = p->extendedBy(m_appdata.id());

    QList<PackageState> ret;
    Q_FOREACH (AppPackageKitResource* r, res) {
        ret += PackageState(r->appstreamId(), r->name(), r->comment(), r->isInstalled());
    }
    return ret;
}

QStringList AppPackageKitResource::extends() const
{
    return m_appdata.extends();
}

void AppPackageKitResource::fetchChangelog()
{
    QString changelog;
    for(const auto& rel: m_appdata.releases()) {
        changelog += QStringLiteral("<h3>") + rel.version() + QStringLiteral("</h3>");
        changelog += QStringLiteral("<p>") + rel.description() + QStringLiteral("</p>");
    }
    emit changelogFetched(changelog);
}
