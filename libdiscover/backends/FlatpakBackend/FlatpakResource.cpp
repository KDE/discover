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

#include <Transaction/AddonList.h>

#include <KLocalizedString>

#include <QDebug>
#include <QDesktopServices>
#include <QIcon>
#include <QStringList>
#include <QTimer>

FlatpakResource::FlatpakResource(AsApp *app, FlatpakBackend *parent)
    : AbstractResource(parent)
    , m_app(app)
    , m_size(0)
{
}

AsApp * FlatpakResource::appstreamApp() const
{
    return m_app;
}

QList<PackageState> FlatpakResource::addonsInformation()
{
    return m_addons;
}

QString FlatpakResource::availableVersion() const
{
    // TODO check if there is actually version available
    QString version = QString::fromUtf8(as_app_get_branch(m_app));
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    return version;
}

QString FlatpakResource::appstreamId() const
{
    return QString::fromUtf8(as_app_get_id(m_app));
}

QString FlatpakResource::arch() const
{
    return m_arch;
}

bool FlatpakResource::canExecute() const
{
    AsAppState appState = as_app_get_state(m_app);
    return (appState == AS_APP_STATE_INSTALLED || appState == AS_APP_STATE_UPDATABLE || appState == AS_APP_STATE_UPDATABLE_LIVE);
}

QStringList FlatpakResource::categories()
{
    QStringList categoriesList;
    GPtrArray *categories = as_app_get_categories(m_app);
    for (uint i = 0; i < categories->len; i++) {
        const char * category = reinterpret_cast<char*>(g_ptr_array_index(categories, i));
        categoriesList += QString::fromUtf8(category);
    }

    qWarning() << categoriesList;

    return categoriesList;
}

QString FlatpakResource::comment()
{
    return QString::fromUtf8(as_app_get_comment(m_app, nullptr));
}

QString FlatpakResource::commit() const
{
    return m_commit;
}

QStringList FlatpakResource::executables() const
{
//     return m_appdata.provided(AppStream::Provided::KindBinary).items();
    return QStringList();
}

QVariant FlatpakResource::icon() const
{
    QIcon ret;
    GPtrArray *icons = as_app_get_icons(m_app);

    if (!icons) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else {
        QStringList stock;
        for (uint i = 0; i < icons->len; i++) {
            AsIcon *icon = AS_ICON(g_ptr_array_index(icons, i));
            AsIconKind iconKind = as_icon_get_kind(icon);
            if (iconKind == AS_ICON_KIND_LOCAL) {
                QString url = QString::fromUtf8(as_icon_get_url(icon));
                // Attempt to contruct the icon url
                if (url.isEmpty()) {
                    url = QString(QLatin1String("%1/icons%2x%3/%4")).arg(QString::fromUtf8(as_app_get_icon_path(m_app))).arg(as_icon_get_width(icon)).arg(as_icon_get_height(icon)).arg(QString::fromUtf8(as_icon_get_name(icon)));
                }
                ret.addFile(url);
            } else if (iconKind == AS_ICON_KIND_CACHED) {
                QString url = QString::fromUtf8(as_icon_get_url(icon));
                // Attempt to contruct the icon url
                if (url.isEmpty()) {
                    url = QString(QLatin1String("%1/icons/%2x%3/%4")).arg(QString::fromUtf8(as_app_get_icon_path(m_app))).arg(as_icon_get_width(icon)).arg(as_icon_get_height(icon)).arg(QString::fromUtf8(as_icon_get_name(icon)));
                }
                ret.addFile(url);
            } else if (iconKind == AS_ICON_KIND_STOCK) {
                stock += QString::fromUtf8(as_icon_get_name(icon));
            }
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
    QString version = QString::fromUtf8(as_app_get_branch(m_app));
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
    return QUrl(QString::fromUtf8(as_app_get_url_item(m_app, AS_URL_KIND_HOMEPAGE)));
}

QString FlatpakResource::flatpakName() const
{
    return m_flatpakName;
}

QString FlatpakResource::license()
{
    return QString::fromUtf8(as_app_get_project_license(m_app));
}

QString FlatpakResource::longDescription()
{
    return QString::fromUtf8(as_app_get_description(m_app, nullptr));
}

QString FlatpakResource::name()
{
    QString name = QString::fromUtf8(as_app_get_name(m_app, nullptr));
    if (name.isEmpty()) {
        name = QString::fromUtf8(as_app_get_id(m_app));
    }

    return name;
}

QString FlatpakResource::origin() const
{
    return QString::fromUtf8(as_app_get_origin(m_app));
}

QString FlatpakResource::packageName() const
{
    return QString::fromUtf8(as_app_get_pkgname_default(m_app));
}

static QUrl imageOfKind(AsScreenshot *screenshot, AsImageKind imageKind)
{
    QUrl ret;
    GPtrArray *images = as_screenshot_get_images(screenshot);

    for (uint i = 0; i < images->len; i++) {
        AsImage *image = AS_IMAGE(g_ptr_array_index(images, i));
        if (as_image_get_kind(image) == imageKind) {
            ret = QUrl(QString::fromUtf8(as_image_get_url(image)));
            break;
        }
    }
    return ret;
}

static QUrl screenshot(AsApp *app, AsImageKind imageKind)
{
    QUrl ret;
    GPtrArray *screenshotsArray = as_app_get_screenshots(app);

    for (uint i = 0; i < screenshotsArray->len; i++) {
        AsScreenshot *screenshot = AS_SCREENSHOT(g_ptr_array_index(screenshotsArray, i));
        ret = imageOfKind(screenshot, imageKind);
        if ((as_screenshot_get_kind(screenshot) == AS_SCREENSHOT_KIND_DEFAULT) && !ret.isEmpty()) {
            break;
        }
    }
    return ret;
}

QUrl FlatpakResource::screenshotUrl()
{
    return screenshot(m_app, AS_IMAGE_KIND_SOURCE);
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
    AsAppState appState = as_app_get_state(m_app);
    if (appState == AS_APP_STATE_INSTALLED) {
        return AbstractResource::Installed;
    } else if (appState == AS_APP_STATE_UPDATABLE || appState == AS_APP_STATE_UPDATABLE_LIVE) {
        return AbstractResource::Upgradeable;
    } else {
        return AbstractResource::None;
    }
}

QUrl FlatpakResource::thumbnailUrl()
{
    return screenshot(m_app, AS_IMAGE_KIND_THUMBNAIL);
}

QString FlatpakResource::uniqueId() const
{
    // Build uniqueId if the bundle kind is not known
    if (!as_app_get_bundle_default(m_app)) {
        return QString::fromUtf8(as_utils_unique_id_build(as_app_get_scope(m_app),
                                                          AS_BUNDLE_KIND_FLATPAK, // Bundle kind should be flatpak for flatpak apps
                                                          as_app_get_origin(m_app),
                                                          as_app_get_kind(m_app),
                                                          as_app_get_id(m_app),
                                                          as_app_get_branch(m_app)));
    }

    // Otherwise the id should have all the necessary information and not use "*" for unknown stuff
    return QString::fromUtf8(as_app_get_unique_id(m_app));
}

void FlatpakResource::invokeApplication() const
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) localError = nullptr;

    const FlatpakBackend *p = static_cast<FlatpakBackend*>(parent());

    if (!flatpak_installation_launch(p->flatpakInstallationForAppScope(as_app_get_scope(m_app)),
                                     m_flatpakName.toStdString().c_str(),
                                     m_arch.toStdString().c_str(),
                                     as_app_get_branch(m_app),
                                     m_commit.toStdString().c_str(),
                                     cancellable,
                                     &localError)) {
        qWarning() << "Failed to launch " << as_app_get_name(m_app, nullptr) << ": " << localError->message;
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
    GPtrArray *screenshotsArray = as_app_get_screenshots(m_app);
    QList<QUrl> thumbnails, screenshots;

    for (uint i = 0; i < screenshotsArray->len; i++) {
        AsScreenshot *screenshot = AS_SCREENSHOT(g_ptr_array_index(screenshotsArray, i));
        const QUrl thumbnail = imageOfKind(screenshot, AS_IMAGE_KIND_THUMBNAIL);
        const QUrl plain = imageOfKind(screenshot, AS_IMAGE_KIND_SOURCE);
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

void FlatpakResource::setCommit(const QString &commit)
{
    m_commit = commit;
}

void FlatpakResource::setFlatpakName(const QString &name)
{
    m_flatpakName = name;
}

void FlatpakResource::setState(AbstractResource::State state)
{
    if (state == AbstractResource::None) {
        as_app_set_state(m_app, AS_APP_STATE_AVAILABLE);
    } else if (state == AbstractResource::Installed) {
        as_app_set_state(m_app, AS_APP_STATE_INSTALLED);
    } else if (state == AbstractResource::Upgradeable) {
        as_app_set_state(m_app, AS_APP_STATE_UPDATABLE);
    }

    emit stateChanged();
}

void FlatpakResource::setSize(int size)
{
    m_size = size;
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
