/***************************************************************************
 *   Copyright © 2010-2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "Application.h"
#include "resources/PackageState.h"
#include "ApplicationBackend.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QVector>
#include <QtCore/QStringList>
#include <QThread>

// KDE includes
#include <KIconLoader>
#include <KLocale>
#include <KService>
#include <KServiceGroup>
#include <KDebug>
#include <KToolInvocation>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KStandardDirs>
#include <KConfigGroup>

// QApt includes
#include <LibQApt/Backend>
#include <LibQApt/Config>

//QJSON includes
#include <qjson/parser.h>

Application::Application(const QString& fileName, QApt::Backend* backend)
        : AbstractResource(0)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(true)
        , m_isTechnical(false)
        , m_isExtrasApp(false)
{
    m_data = desktopContents(fileName);
    m_isTechnical = getField("NoDisplay").toLower() == "true" || !hasField("Exec");
    m_packageName = getField("X-AppInstall-Package");
}

Application::Application(QApt::Package* package, QApt::Backend* backend)
        : AbstractResource(0)
        , m_backend(backend)
        , m_package(package)
        , m_isValid(true)
        , m_isTechnical(true)
        , m_isExtrasApp(false)
{
    m_packageName = m_package->name().latin1();
    
    if (m_package->architecture() != m_backend->nativeArchitecture())
        m_packageName.append(QByteArray(":") + m_package->architecture().toLatin1());

    if (m_package->origin() == QLatin1String("LP-PPA-app-review-board")) {
        if (!m_package->controlField(QLatin1String("Appname")).isEmpty()) {
            m_isExtrasApp = true;
            m_isTechnical = false;
        }
    }
}

QString Application::name()
{
    if (!m_isTechnical)
        return i18n(untranslatedName().toUtf8());

    // Technical packages use the package name, which is untranslatable
    return untranslatedName();
}

QString Application::untranslatedName()
{
    QString name = QString::fromUtf8(getField("Name")).trimmed();
    if (name.isEmpty() && package()) {
        // extras.ubuntu.com packages can have this
        if (m_isExtrasApp)
            name = m_package->controlField(QLatin1String("Appname"));
        else
            name = m_package->name();
    }

    return name;
}

QString Application::comment()
{
    QString comment = getField("Comment");
    if (comment.isEmpty()) {
        // Sometimes GenericName is used instead of Comment
        comment = getField("GenericName");
        if (comment.isEmpty()) {
            return package()->shortDescription();
        }
    }

    return i18n(comment.toUtf8());
}

QString Application::packageName() const
{
    return m_packageName;
}

QApt::Package *Application::package()
{
    if (!m_package && m_backend) {
        m_package = m_backend->package(packageName());
        emit stateChanged();
    }

    // Packages removed from archive will remain in app-install-data until the
    // next refresh, so we can have valid .desktops with no package
    if (!m_package) {
        m_isValid = false;
    }

    return m_package;
}

QString Application::icon() const
{
    return getField("Icon", "applications-other");
}

QString Application::mimetypes() const
{
    return getField("MimeType");
}

QString Application::menuPath()
{
    QString path;
    QString arrow(QString::fromUtf8(" ➜ "));

    // Take the file name and remove the .desktop ending
    QVector<KService::Ptr> execs = executables();
    if(execs.isEmpty())
        return path;

    KService::Ptr service = execs.first();
    QVector<QPair<QString, QString> > ret;

    if (service) {
        ret = locateApplication(QString(), service->menuId());
    }

    if (!ret.isEmpty()) {
        path.append(QString("<img width=\"16\" heigh=\"16\"src=\"%1\"/>")
                    .arg(KIconLoader::global()->iconPath("kde", KIconLoader::Small)));
        path.append(QString("&nbsp;%1 <img width=\"16\" heigh=\"16\" src=\"%2\"/>&nbsp;%3")
                    .arg(arrow)
                    .arg(KIconLoader::global()->iconPath("applications-other", KIconLoader::Small))
                    .arg(i18n("Applications")));
        for (int i = 0; i < ret.size(); i++) {
            path.append(QString("&nbsp;%1&nbsp;<img width=\"16\" heigh=\"16\" src=\"%2\"/>&nbsp;%3")
                        .arg(arrow)
                        .arg(KIconLoader::global()->iconPath(ret.at(i).second, KIconLoader::Small))
                        .arg(ret.at(i).first));
        }
    }

    return path;
}

QVector<QPair<QString, QString> > Application::locateApplication(const QString &_relPath, const QString &menuId) const
{
    QVector<QPair<QString, QString> > ret;
    KServiceGroup::Ptr root = KServiceGroup::group(_relPath);

    if (!root || !root->isValid()) {
        return ret;
    }

    const KServiceGroup::List list = root->entries(false /* sorted */,
                                                   true /* exclude no display entries */,
                                                   false /* allow separators */);

    for (KServiceGroup::List::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        const KSycocaEntry::Ptr p = (*it);

        if (p->isType(KST_KService)) {
            const KService::Ptr service = KService::Ptr::staticCast(p);

            if (service->noDisplay()) {
                continue;
            }

            if (service->menuId() == menuId) {
                QPair<QString, QString> pair;
                pair.first  = service->name();
                pair.second = service->icon();
                ret << pair;
                return ret;
            }
        } else if (p->isType(KST_KServiceGroup)) {
            const KServiceGroup::Ptr serviceGroup = KServiceGroup::Ptr::staticCast(p);

            if (serviceGroup->noDisplay() || serviceGroup->childCount() == 0) {
                continue;
            }

            QVector<QPair<QString, QString> > found;
            found = locateApplication(serviceGroup->relPath(), menuId);
            if (!found.isEmpty()) {
                QPair<QString, QString> pair;
                pair.first  = serviceGroup->caption();
                pair.second = serviceGroup->icon();
                ret << pair;
                ret << found;
                return ret;
            }
        } else {
            continue;
        }
    }

    return ret;
}

QString Application::categories()
{
    QString categories = getField("Categories");

    if (categories.isEmpty()) {
        // extras.ubuntu.com packages can have this field
        if (m_isExtrasApp)
            categories = package()->controlField(QLatin1String("Category"));
    }
    return categories;
}

QUrl Application::screenshotUrl(QApt::ScreenshotType type)
{
    return package()->screenshotUrl(type);
}

QString Application::license()
{
    if (package()->component() == "main" ||
        package()->component() == "universe") {
        return i18nc("@info license", "Open Source");
    } else if (package()->component() == "restricted") {
        return i18nc("@info license", "Proprietary");
    } else {
        return i18nc("@info license", "Unknown");
    }
}

QApt::PackageList Application::addons()
{
    QApt::PackageList addons;

    QApt::Package *pkg = package();
    if (!pkg) {
        return addons;
    }

    QStringList tempList;
    // Only add recommends or suggests to the list if they aren't already going to be
    // installed
    if (!m_backend->config()->readEntry("APT::Install-Recommends", true)) {
        tempList << m_package->recommendsList();
    }
    if (!m_backend->config()->readEntry("APT::Install-Suggests", false)) {
        tempList << m_package->suggestsList();
    }
    tempList << m_package->enhancedByList();

    QStringList languagePackages;
    QFile l10nFilterFile("/usr/share/language-selector/data/pkg_depends");

    if (l10nFilterFile.open(QFile::ReadOnly)) {
        QString contents = l10nFilterFile.readAll();

        foreach (const QString &line, contents.split('\n')) {
            if (line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            languagePackages << line.split(':').last();
        }

        languagePackages.removeAll("");
    }

    foreach (const QString &addon, tempList) {
        bool shouldShow = true;
        QApt::Package *package = m_backend->package(addon);

        if (!package || QString(package->section()).contains("lib") || addons.contains(package)) {
            continue;
        }

        foreach (const QString &langpack, languagePackages) {
            if (addon.contains(langpack)) {
                shouldShow = false;
                break;
            }
        }

        if (shouldShow) {
            addons << package;
        }
    }

    return addons;
}

QList<PackageState> Application::addonsInformation()
{
    QList<PackageState> ret;
    QApt::PackageList pkgs = addons();
    foreach(QApt::Package* p, pkgs) {
        ret += PackageState(p->name(), p->shortDescription(), p->isInstalled());
    }
    return ret;
}

bool Application::isValid() const
{
    return m_isValid;
}

bool Application::isTechnical() const
{
    return m_isTechnical;
}

QByteArray Application::getField(const char* field, const QByteArray& defaultvalue) const
{
    if(m_data) {
        KConfigGroup group = m_data->group("Desktop Entry");
        return group.readEntry(field, defaultvalue);
    } else
        return defaultvalue;

}

bool Application::hasField(const char* field) const
{
    return m_data && m_data->group("Desktop Entry").hasKey(field);
}

QUrl Application::homepage() const
{
    if(!m_package) return QString();
    return m_package->homepage();
}

QString Application::origin() const
{
    if(!m_package) return QString();
    return m_package->origin();
}

QString Application::longDescription() const
{
    if(!m_package) return QString();
    return m_package->longDescription();
}

QString Application::availableVersion() const
{
    if(!m_package) return QString();
    return m_package->availableVersion();
}

QString Application::installedVersion() const
{
    if(!m_package) return QString();
    return m_package->installedVersion();
}

QString Application::sizeDescription()
{
    if (!isInstalled()) {
        return i18nc("@info app size", "%1 to download, %2 on disk",
                              KGlobal::locale()->formatByteSize(package()->downloadSize()),
                              KGlobal::locale()->formatByteSize(package()->availableInstalledSize()));
    } else {
        return i18nc("@info app size", "%1 on disk",
                              KGlobal::locale()->formatByteSize(package()->currentInstalledSize()));
    }
}

KSharedConfigPtr Application::desktopContents(const QString& filename)
{
    return KSharedConfig::openConfig(filename, KConfig::SimpleConfig);
}

void Application::clearPackage()
{
    m_package = 0;
}

QVector<KService::Ptr> Application::executables() const
{
    QVector<KService::Ptr> ret;
    foreach (const QString &desktop, m_package->installedFilesList().filter(".desktop")) {
        // we create a new KService because findByDestopPath
        // might fail because the Sycoca database is not up to date yet.
        KService::Ptr service = KService::serviceByStorageId(desktop);
        if (service->isApplication() &&
            !service->noDisplay() &&
            !service->exec().isEmpty())
        {
            ret << service;
        }
    }
    return ret;
}

void Application::emitStateChanged()
{
    emit stateChanged();
}

void Application::invokeApplication() const
{
    QVector< KService::Ptr > execs = executables();
    Q_ASSERT(!execs.isEmpty());
    KToolInvocation::startServiceByDesktopPath(execs.first()->desktopEntryPath());
}

bool Application::canExecute() const
{
    return !executables().isEmpty();
}

QString Application::section()
{
    return package()->section();
}

int Application::popularityContest() const
{
    return getField("X-AppInstall-Popcon").toInt();
}

AbstractResource::State Application::state()
{
    State ret = None;
    int s = package()->state();
    if(s & QApt::Package::Upgradeable) ret = Upgradeable;
    else if(s & QApt::Package::Installed) ret = Installed;
    
    return ret;
}

void Application::fetchScreenshots()
{
    QString dest = KStandardDirs::locate("tmp", "screenshots."+m_packageName);
    
    //TODO: Make async
    bool done = false;
    bool ret = KIO::NetAccess::download(KUrl("http://screenshots.debian.net/json/package/"+m_packageName), dest, 0);
    if(ret) {
        QFile f(dest);
        f.open(QIODevice::ReadOnly);
        
        QJson::Parser p;
        bool ok;
        QVariantMap values = p.parse(&f, &ok).toMap();
        if(!ok) {
            QVariantList screenshots = values["screenshots"].toList();
            
            QList<QUrl> thumbnailUrls, screenshotUrls;
            foreach(const QVariant& screenshot, screenshots) {
                QVariantMap s = screenshot.toMap();
                thumbnailUrls += s["small_image_url"].toUrl();
                screenshotUrls += s["large_image_url"].toUrl();
            }
            emit screenshotsFetched(thumbnailUrls, screenshotUrls);
            done = true;
        }
    }
    if(!done)
        emit screenshotsFetched(QList<QUrl>() << thumbnailUrl(), QList<QUrl>() << screenshotUrl());
}
