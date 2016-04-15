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

// Qt includes
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QVector>

// KDE includes
#include <KIconLoader>
#include <KService>
#include <KServiceGroup>
#include <KToolInvocation>
#include <KLocalizedString>
#include <KIO/Job>
#include <KConfigGroup>
#include <KFormat>

// QApt includes
#include <QApt/Backend>
#include <QApt/Config>
#include <QApt/Changelog>
#include <qapt/qaptversion.h>

#include <MuonDataSources.h>

#include "ApplicationBackend.h"
#include "resources/PackageState.h"

Application::Application(const Appstream::Component &component, QApt::Backend* backend)
        : AbstractResource(0)
        , m_data(component)
        , m_package(0)
        , m_isValid(true)
        , m_isTechnical(component.kind() != Appstream::Component::KindDesktop)
        , m_isExtrasApp(false)
        , m_sourceHasScreenshot(true)
{
    static QByteArray currentDesktop = qgetenv("XDG_CURRENT_DESKTOP");

    Q_ASSERT(component.packageNames().count() == 1);
    if (!component.packageNames().empty())
        m_packageName = component.packageNames().at(0);
    
    m_package = backend->package(packageName());
    m_isValid = bool(m_package);
}

Application::Application(QApt::Package* package, QApt::Backend* backend)
        : AbstractResource(0)
        , m_package(package)
        , m_packageName(m_package->name())
        , m_isValid(true)
        , m_isTechnical(true)
        , m_isExtrasApp(false)
{
    QString arch = m_package->architecture();
    if (arch != backend->nativeArchitecture() && arch != QLatin1String("all")) {
        m_packageName.append(QLatin1Char(':'));
        m_packageName.append(arch);
    }

    if (m_package->origin() == QLatin1String("LP-PPA-app-review-board")) {
        if (!m_package->controlField(QLatin1String("Appname")).isEmpty()) {
            m_isExtrasApp = true;
            m_isTechnical = false;
        }
    }
}

QString Application::name()
{
    QString name = m_data.isValid() ? m_data.name() : QString();
    if (name.isEmpty() && package()) {
        // extras.ubuntu.com packages can have this
        if (m_isExtrasApp)
            name = m_package->controlField(QLatin1String("Appname"));
        else
            name = m_package->name();
    }

    if (package() && m_package->isForeignArch())
        name = i18n("%1 (%2)", name, m_package->architecture());
    
    return name;
}

QString Application::comment()
{
    QString comment = m_data.isValid() ? m_data.description() : QString();
    if (comment.isEmpty()) {
        return package()->shortDescription();
    }

    return i18n(comment.toUtf8().constData());
}

QString Application::packageName() const
{
    return m_packageName;
}

QApt::Package *Application::package()
{
    if (!m_package && parent()) {
        m_package = backend()->package(packageName());
        Q_EMIT stateChanged();
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
    QString anIcon = m_data.icon();
    if (anIcon.isEmpty()) {
        QUrl iconUrl = m_data.iconUrl(QSize());
        if (iconUrl.isLocalFile())
            anIcon = iconUrl.toLocalFile();
    }
    return anIcon;
}

QStringList Application::findProvides(Appstream::Provides::Kind kind) const
{
    QStringList ret;
    Q_FOREACH (Appstream::Provides p, m_data.provides())
        if (p.kind() == kind)
            ret += p.value();
    return ret;
}

QStringList Application::mimetypes() const
{
    return findProvides(Appstream::Provides::KindMimetype);
}

QString Application::menuPath()
{
    QString path;
    QString arrow(QString::fromUtf8(" ➜ "));

    // Take the file name and remove the .desktop ending
    QVector<KService::Ptr> execs = findExecutables();
    if(execs.isEmpty())
        return path;

    KService::Ptr service = execs.first();
    QVector<QPair<QString, QString> > ret;

    if (service) {
        ret = locateApplication(QString(), service->menuId());
    }

    if (!ret.isEmpty()) {
        path.append(QStringLiteral("<img width=\"16\" height=\"16\"src=\"%1\"/>")
                    .arg(KIconLoader::global()->iconPath(QStringLiteral("kde"), KIconLoader::Small)));
        path.append(QStringLiteral("&nbsp;%1 <img width=\"16\" height=\"16\" src=\"%2\"/>&nbsp;%3")
                    .arg(arrow)
                    .arg(KIconLoader::global()->iconPath(QStringLiteral("applications-other"), KIconLoader::Small))
                    .arg(i18n("Applications")));
        for (int i = 0; i < ret.size(); i++) {
            path.append(QStringLiteral("&nbsp;%1&nbsp;<img width=\"16\" height=\"16\" src=\"%2\"/>&nbsp;%3")
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

        // Static cast to specific classes according to isType().
        if (p->isType(KST_KService)) {
            const KService::Ptr service =
                    KService::Ptr(static_cast<KService *>(p.data()));

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
            const KServiceGroup::Ptr serviceGroup =
                    KServiceGroup::Ptr(static_cast<KServiceGroup *>(p.data()));

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

QStringList Application::categories()
{
    QStringList categories = m_data.isValid() ? m_data.categories() : QStringList();

    if (categories.isEmpty()) {
        // extras.ubuntu.com packages can have this field
        if (m_isExtrasApp)
            categories = package()->controlField(QLatin1String("Category")).split(QLatin1Char(';'));
    }
    return categories;
}

QUrl Application::thumbnailUrl()
{
    QUrl url(package()->controlField(QLatin1String("Thumbnail-Url")));
    if(m_sourceHasScreenshot) {
        url = QUrl(MuonDataSources::screenshotsSource().toString() + QStringLiteral("/thumbnail/") + packageName());
    }
    return url;
}

QUrl Application::screenshotUrl()
{
    QUrl url(package()->controlField(QLatin1String("Screenshot-Url")));
    if(m_sourceHasScreenshot) {
        url = QUrl(MuonDataSources::screenshotsSource().toString() + QStringLiteral("/screenshot/") + packageName());
    }
    return url;
}

QString Application::license()
{
    QString component = package()->component();
    if (component == QLatin1String("main") || component == QLatin1String("universe")) {
        return i18nc("@info license", "Open Source");
    } else if (component == QLatin1String("restricted")) {
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
    if (!backend()->config()->readEntry(QStringLiteral("APT::Install-Recommends"), true)) {
        tempList << m_package->recommendsList();
    }
    if (!backend()->config()->readEntry(QStringLiteral("APT::Install-Suggests"), false)) {
        tempList << m_package->suggestsList();
    }
    tempList << m_package->enhancedByList();

    QStringList languagePackages;
    QFile l10nFilterFile(QStringLiteral("/usr/share/language-selector/data/pkg_depends"));

    if (l10nFilterFile.open(QFile::ReadOnly)) {
        QString contents = QString::fromLatin1(l10nFilterFile.readAll());

        foreach (const QString &line, contents.split(QLatin1Char('\n'))) {
            if (line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            languagePackages << line.split(QLatin1Char(':')).last();
        }

        languagePackages.removeAll(QString());
    }

    foreach (const QString &addon, tempList) {
        bool shouldShow = true;
        QApt::Package *package = backend()->package(addon);

        if (!package || QString(package->section()).contains(QLatin1String("lib")) || addons.contains(package)) {
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

QUrl Application::homepage()
{
    if(!m_package) return QUrl();
    return QUrl(m_package->homepage());
}

QString Application::origin() const
{
    if(!m_package) return QString();
    return m_package->origin();
}

QString Application::longDescription()
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
    KFormat f;
    if (!isInstalled()) {
        return i18nc("@info app size", "%1 to download, %2 on disk",
                              f.formatByteSize(package()->downloadSize()),
                              f.formatByteSize(package()->availableInstalledSize()));
    } else {
        return i18nc("@info app size", "%1 on disk",
                              f.formatByteSize(package()->currentInstalledSize()));
    }
}

int Application::size()
{
    return m_package->downloadSize();
}

void Application::clearPackage()
{
    m_package = 0;
}

QVector<KService::Ptr> Application::findExecutables() const
{
    QVector<KService::Ptr> ret;
    if (!m_package) {
        qWarning() << "trying to find the executables for an uninitialized package!" << packageName();
        return ret;
    }

    QRegExp rx(QStringLiteral(".+\\.desktop$"), Qt::CaseSensitive);
    foreach (const QString &desktop, m_package->installedFilesList().filter(rx)) {
        // Important to use serviceByStorageId to ensure we get a service even
        // if the KSycoca database doesn't have our .desktop file yet.
        KService::Ptr service = KService::serviceByStorageId(desktop);
        if (service &&
            service->isApplication() &&
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
    QVector< KService::Ptr > execs = findExecutables();
    Q_ASSERT(!execs.isEmpty());
    KToolInvocation::startServiceByDesktopName(execs.first()->desktopEntryName());
}

bool Application::canExecute() const
{
    return !findExecutables().isEmpty();
}

QString Application::section()
{
    return package()->section();
}

AbstractResource::State Application::state()
{
    if (!package())
        return Broken;

    int s = package()->state();

    if (s & QApt::Package::Upgradeable) {
#if QAPT_VERSION >= QT_VERSION_CHECK(3, 1, 0)
        if (package()->isInUpdatePhase())
            return Upgradeable;
#else
        return Upgradeable;
#endif
    }

    if (s & QApt::Package::Installed) {
        return Installed;
    }
    
    return None; // Actually: none of interest to us here in plasma-discover.
}

void Application::fetchScreenshots()
{
    if(!m_sourceHasScreenshot)
        return;
    
    QString dest = QStandardPaths::locate(QStandardPaths::TempLocation, QStringLiteral("screenshots.")+m_packageName);
    const QUrl packageUrl(MuonDataSources::screenshotsSource().toString() + QStringLiteral("/json/package/")+m_packageName);
    KIO::StoredTransferJob* job = KIO::storedGet(packageUrl, KIO::NoReload, KIO::HideProgressInfo);
    connect(job, &KIO::StoredTransferJob::finished, this, &Application::downloadingScreenshotsFinished);
}

void Application::downloadingScreenshotsFinished(KJob* j)
{
    KIO::StoredTransferJob* job = qobject_cast< KIO::StoredTransferJob* >(j);
    bool done = false;
    if(job) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(job->data(), &error);
        if(error.error != QJsonParseError::NoError) {
            QVariantMap values = doc.toVariant().toMap();
            QVariantList screenshots = values[QStringLiteral("screenshots")].toList();
            
            QList<QUrl> thumbnailUrls, screenshotUrls;
            foreach(const QVariant& screenshot, screenshots) {
                QVariantMap s = screenshot.toMap();
                thumbnailUrls += s[QStringLiteral("small_image_url")].toUrl();
                screenshotUrls += s[QStringLiteral("large_image_url")].toUrl();
            }
            emit screenshotsFetched(thumbnailUrls, screenshotUrls);
            done = true;
        }
    }
    if(!done) {
        QList<QUrl> thumbnails, screenshots;
        if(!thumbnailUrl().isEmpty()) {
            thumbnails += thumbnailUrl();
            screenshots += screenshotUrl();
        }
        emit screenshotsFetched(thumbnails, screenshots);
    }

}

void Application::setHasScreenshot(bool has)
{
    m_sourceHasScreenshot = has;
}

QStringList Application::executables() const
{
    QStringList ret;
    const QVector<KService::Ptr> exes = findExecutables();
    for(KService::Ptr exe : exes) {
        ret += exe->exec();
    }
    return ret;
}

bool Application::isFromSecureOrigin() const
{
    Q_FOREACH (const QString &archive, m_package->archives()) {
        if (archive.contains(QLatin1String("security"))) {
            return true;
        }
    }
    return false;
}

void Application::fetchChangelog()
{
    KIO::StoredTransferJob* getJob = KIO::storedGet(m_package->changelogUrl(), KIO::NoReload, KIO::HideProgressInfo);
    connect(getJob, &KIO::StoredTransferJob::result, this, &Application::processChangelog);
}

void Application::processChangelog(KJob* j)
{
    KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);
    if (!m_package || !job) {
        return;
    }

    QString changelog;
    if(j->error()==0)
        changelog = buildDescription(job->data(), m_package->sourcePackage());

    if (changelog.isEmpty()) {
        if (m_package->origin() == QStringLiteral("Ubuntu")) {
            changelog = xi18nc("@info/rich", "The list of changes is not yet available. "
                                             "Please use <link url='%1'>Launchpad</link> instead.",
                                             QStringLiteral("http://launchpad.net/ubuntu/+source/") + m_package->sourcePackage());
        } else {
            changelog = xi18nc("@info", "The list of changes is not yet available.");
        }
    }
    emit changelogFetched(changelog);
}

QString Application::buildDescription(const QByteArray& data, const QString& source)
{
    QApt::Changelog changelog(QString::fromLatin1(data), source);
    QString description;

    QApt::ChangelogEntryList entries = changelog.newEntriesSince(m_package->installedVersion());

    if (entries.size() < 1) {
        return description;
    }

    foreach(const QApt::ChangelogEntry &entry, entries) {
        description += i18nc("@info:label Refers to a software version, Ex: Version 1.2.1:",
                             "Version %1:", entry.version());

        KFormat f;
        QString issueDate = entry.issueDateTime().toString(Qt::DefaultLocaleShortDate);
        description += QLatin1String("<p>") +
                       i18nc("@info:label", "This update was issued on %1", issueDate) +
                       QLatin1String("</p>");

        QString updateText = entry.description();
        updateText.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
        description += QLatin1String("<p><pre>") + updateText + QLatin1String("</pre></p>");
    }

    return description;
}

QApt::Backend *Application::backend() const
{
    return qobject_cast<ApplicationBackend*>(parent())->backend();
}
