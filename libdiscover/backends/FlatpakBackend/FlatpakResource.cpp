/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "FlatpakResource.h"
#include "FlatpakBackend.h"

#include <Transaction/AddonList.h>

#include <AppStreamQt/icon.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/screenshot.h>

#include <KLocalizedString>

#include <QDebug>
#include <QDesktopServices>
#include <QIcon>
#include <QStringList>
#include <QTimer>

FlatpakResource::FlatpakResource(AppStream::Component *component, FlatpakBackend *parent)
    : AbstractResource(parent)
    , m_appdata(component)
    , m_scope(FlatpakResource::System)
    , m_size(0)
    , m_state(AbstractResource::None)
    , m_type(FlatpakResource::DesktopApp)
{
}

AppStream::Component *FlatpakResource::appstreamComponent() const
{
    return m_appdata;
}

QList<PackageState> FlatpakResource::addonsInformation()
{
    return m_addons;
}

QString FlatpakResource::availableVersion() const
{
    // TODO check if there is actually version available
    QString version = branch();
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    return version;
}

QString FlatpakResource::appstreamId() const
{
    return m_appdata->id();
}

QString FlatpakResource::arch() const
{
    return m_arch;
}

QString FlatpakResource::branch() const
{
    return m_branch;
}

bool FlatpakResource::canExecute() const
{
    return (m_state == AbstractResource::Installed || m_state == AbstractResource::Upgradeable);
}

QStringList FlatpakResource::categories()
{
    auto cats = m_appdata->categories();
    if (m_appdata->kind() != AppStream::Component::KindAddon)
        cats.append(QStringLiteral("Application"));
    return cats;
}

QString FlatpakResource::comment()
{
    const auto summary = m_appdata->summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return QString();
}

QString FlatpakResource::commit() const
{
    return m_commit;
}

QStringList FlatpakResource::executables() const
{
//     return m_appdata->provided(AppStream::Provided::KindBinary).items();
    return QStringList();
}

QVariant FlatpakResource::icon() const
{
    QIcon ret;
    const auto icons = m_appdata->icons();

    if (icons.isEmpty()) {
        qWarning() << "empty";
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else foreach(const AppStream::Icon &icon, icons) {
        QStringList stock;
        QString url = QString::fromUtf8("%1/icons/").arg(m_iconPath);

        switch (icon.kind()) {
            case AppStream::Icon::KindLocal:
                url += icon.url().toLocalFile();
                ret.addFile(url);
                break;
            case AppStream::Icon::KindCached:
                url += icon.url().toLocalFile();
                ret.addFile(url);
                break;
            case AppStream::Icon::KindStock:
                stock += icon.name();
                break;
        }

        if (ret.isNull() && !stock.isEmpty()) {
            ret = QIcon::fromTheme(stock.first(), QIcon::fromTheme(QStringLiteral("package-x-generic")));
        }
    }
    return ret;
}

QString FlatpakResource::installedVersion() const
{
    // TODO check if there is actually version available
    QString version = branch();
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    return version;
}

bool FlatpakResource::isTechnical() const
{
    return false;
}

QUrl FlatpakResource::homepage()
{
    return m_appdata->url(AppStream::Component::UrlKindHomepage);
}

QString FlatpakResource::flatpakName() const
{
    // If the flatpak name is not known (known only for installed apps), then use
    // appstream id instead;
    if (m_flatpakName.isEmpty()) {
        return m_appdata->id();
    }

    return m_flatpakName;
}

QString FlatpakResource::license()
{
    return m_appdata->projectLicense();
}

QString FlatpakResource::longDescription()
{
    return m_appdata->description();
}

QString FlatpakResource::name()
{
    QString name = m_appdata->name();
    if (name.isEmpty()) {
        name = m_appdata->id();
    }

    return name;
}

QString FlatpakResource::origin() const
{
    return m_origin;
}

QString FlatpakResource::packageName() const
{
    return m_appdata->name();
}

QString FlatpakResource::runtime() const
{
    return m_runtime;
}

static QUrl imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
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

static QUrl screenshot(AppStream::Component *comp, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Screenshot &s, comp->screenshots()) {
        ret = imageOfKind(s.images(), kind);
        if (s.isDefault() && !ret.isEmpty())
            break;
    }
    return ret;
}

FlatpakResource::Scope FlatpakResource::scope() const
{
    return m_scope;
}

QString FlatpakResource::scopeAsString() const
{
    return m_scope == System ? QLatin1String("system") : QLatin1String("user");
}

QUrl FlatpakResource::screenshotUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindSource);
}

QString FlatpakResource::section()
{
    return QString();
}

int FlatpakResource::size()
{
    return m_size;
}

AbstractResource::State FlatpakResource::state()
{
    return m_state;
}

QUrl FlatpakResource::thumbnailUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindThumbnail);
}

FlatpakResource::ResourceType FlatpakResource::type() const
{
    return m_type;
}

QString FlatpakResource::typeAsString() const
{
    switch (m_type) {
        case FlatpakResource::DesktopApp:
            return QLatin1String("app");
            break;
        case FlatpakResource::Runtime:
            return QLatin1String("runtime");
            break;
    }

    return QLatin1String("app");
}

QString FlatpakResource::uniqueId() const
{
    // Build uniqueId
    const QString scope = m_scope == System ? QLatin1String("system") : QLatin1String("user");
    return QString::fromUtf8("%1/%2/%3/%4/%5/%6").arg(scope)
                                                 .arg(QLatin1String("flatpak"))
                                                 .arg(origin())
                                                 .arg(typeAsString())
                                                 .arg(m_appdata->id())
                                                 .arg(branch());
}

void FlatpakResource::invokeApplication() const
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) localError = nullptr;

    const FlatpakBackend *p = static_cast<FlatpakBackend*>(parent());

    if (!flatpak_installation_launch(p->flatpakInstallationForAppScope(scope()),
                                     flatpakName().toStdString().c_str(),
                                     arch().toStdString().c_str(),
                                     branch().toStdString().c_str(),
                                     nullptr,
                                     cancellable,
                                     &localError)) {
        qWarning() << "Failed to launch " << m_appdata->name() << ": " << localError->message;
    }
}

void FlatpakResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    emit changelogFetched(log);
}

void FlatpakResource::fetchScreenshots()
{
    QList<QUrl> thumbnails, screenshots;

    Q_FOREACH (const AppStream::Screenshot &s, m_appdata->screenshots()) {
        const QUrl thumbnail = imageOfKind(s.images(), AppStream::Image::KindThumbnail);
        const QUrl plain = imageOfKind(s.images(), AppStream::Image::KindSource);
        if (plain.isEmpty())
            qWarning() << "invalid screenshot for" << name();

        screenshots << plain;
        thumbnails << (thumbnail.isEmpty() ? plain : thumbnail);
    }

    Q_EMIT screenshotsFetched(thumbnails, screenshots);
}

void FlatpakResource::setArch(const QString &arch)
{
    m_arch = arch;
}

void FlatpakResource::setBranch(const QString &branch)
{
    m_branch = branch;
}

void FlatpakResource::setCommit(const QString &commit)
{
    m_commit = commit;
}

void FlatpakResource::setFlatpakName(const QString &name)
{
    m_flatpakName = name;
}

void FlatpakResource::setIconPath(const QString &path)
{
    m_iconPath = path;
}

void FlatpakResource::setOrigin(const QString &origin)
{
    m_origin = origin;
}

void FlatpakResource::setRuntime(const QString &runtime)
{
    m_runtime = runtime;
}

void FlatpakResource::setScope(FlatpakResource::Scope scope)
{
    m_scope = scope;
}

void FlatpakResource::setState(AbstractResource::State state)
{
    m_state = state;

    emit stateChanged();
}

void FlatpakResource::setSize(int size)
{
    m_size = size;
}

void FlatpakResource::setType(FlatpakResource::ResourceType type)
{
    m_type = type;
}

// void FlatpakResource::setAddons(const AddonList& addons)
// {
//     Q_FOREACH (const QString& toInstall, addons.addonsToInstall()) {
//         setAddonInstalled(toInstall, true);
//     }
//     Q_FOREACH (const QString& toRemove, addons.addonsToRemove()) {
//         setAddonInstalled(toRemove, false);
//     }
// }

// void FlatpakResource::setAddonInstalled(const QString& addon, bool installed)
// {
//     for(auto & elem : m_addons) {
//         if(elem.name() == addon) {
//             elem.setInstalled(installed);
//         }
//     }
// }
