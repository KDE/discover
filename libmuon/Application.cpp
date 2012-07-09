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

// QApt includes
#include <LibQApt/Backend>
#include <LibQApt/Config>

//QJSON includes
#include <qjson/parser.h>

//QtZeitgeist includes
#include "HaveQZeitgeist.h"
#ifdef HAVE_QZEITGEIST
#include <QtZeitgeist/DataModel/Event>
#include <QtZeitgeist/DataModel/TimeRange>
#include <QtZeitgeist/Log>
#include <QtZeitgeist/Interpretation>
#include <QtZeitgeist/Manifestation>
#include <QtZeitgeist/QtZeitgeist>
#endif

Application::Application(const QString &fileName, QApt::Backend *backend)
        : m_fileName(fileName)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(false)
        , m_isTechnical(false)
        , m_isExtrasApp(false)
        , m_usageCount(-1)
{
    m_data = desktopContents();
    m_isTechnical = m_data.value("NoDisplay").toLower() == "true" || !m_data.contains("Exec");
    m_packageName = getField("X-AppInstall-Package");
}

Application::Application(QApt::Package *package, QApt::Backend *backend)
        : m_backend(backend)
        , m_package(package)
        , m_isValid(true)
        , m_isTechnical(true)
        , m_isExtrasApp(false)
        , m_usageCount(-1)
{
    m_packageName = m_package->latin1Name().latin1();
    if (!m_package->controlField(QLatin1String("Appname")).isEmpty()) {
        m_isExtrasApp = true;
        m_isTechnical = false;
    }
}

Application::~Application()
{
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
            name = m_package->latin1Name();
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
        // kDebug() << m_fileName;
    }

    return m_package;
}

QString Application::icon() const
{
    QString icon = getField("Icon");

    if (icon.isEmpty()) {
        icon = QLatin1String("applications-other");
    }

    return icon;
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
    QString desktopName = m_fileName.split('/').last().split(':').first();
    KService::Ptr service = KService::serviceByDesktopName(desktopName);
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

        if (!package || package->section().contains("lib") || addons.contains(package)) {
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

QByteArray Application::getField(const QByteArray &field) const
{
    return m_data.value(field);
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

QHash<QByteArray, QByteArray> Application::desktopContents()
{
    QHash<QByteArray, QByteArray> contents;

    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        return contents;
    }

    while(!file.atEnd()) {
        QByteArray line = file.readLine();
        line.chop(1);
        
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Looking for a group heading.
        m_isValid = m_isValid || (line.startsWith('[') && line.contains(']'));

        int eqpos = line.indexOf('=');

        if (eqpos >= 0) {
            QByteArray aKey = line.left(eqpos);

            if (!contents.contains(aKey)) {
                QByteArray aValue = line.mid(eqpos+1);
                contents[aKey] = aValue;
            }
        }
    }

    return contents;
}

void Application::populateZeitgeistInfo()
{
    m_usageCount = -1;
#ifdef HAVE_QZEITGEIST
    QString desktopFile;

    foreach (const QString &desktop, package()->installedFilesList().filter(".desktop")) {
        KService::Ptr service = KService::serviceByDesktopPath(desktop);
        if (!service) {
            continue;
        }

        if (service->isApplication() &&
            !service->noDisplay() &&
            !service->exec().isEmpty())
        {
            desktopFile = desktop.split('/').last();
            break;
        }
    }

    QtZeitgeist::init();

    // Prepare the Zeitgeist query
    QtZeitgeist::DataModel::EventList eventListTemplate;

    QtZeitgeist::DataModel::Event event1;
    event1.setActor("application://" + desktopFile);
    event1.setInterpretation(QtZeitgeist::Interpretation::Event::ZGModifyEvent);
    event1.setManifestation(QtZeitgeist::Manifestation::Event::ZGUserActivity);

    QtZeitgeist::DataModel::Event event2;
    event2.setActor("application://" + desktopFile);
    event2.setInterpretation(QtZeitgeist::Interpretation::Event::ZGCreateEvent);
    event2.setManifestation(QtZeitgeist::Manifestation::Event::ZGUserActivity);

    eventListTemplate << event1 << event2;

    QtZeitgeist::Log log;
    QDBusPendingReply<QtZeitgeist::DataModel::EventIdList> reply = log.findEventIds(QtZeitgeist::DataModel::TimeRange::always(),
                                                                                    eventListTemplate,
                                                                                    QtZeitgeist::Log::Any,
                                                                                    0, QtZeitgeist::Log::MostRecentEvents);
    reply.waitForFinished();

    if (reply.isValid()) {
        m_usageCount = reply.value().size();
    }
#endif // HAVE_QZEITGEIST
}

int Application::usageCount()
{
    if(m_usageCount<0)
        populateZeitgeistInfo();
    return m_usageCount;
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
        KService::Ptr service(new KService(desktop));
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

QUrl Application::thumbnailUrl()
{
    return screenshotUrl(QApt::Thumbnail);
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
