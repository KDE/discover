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
#include <QtCore/QFile>
#include <QtCore/QVector>
#include <QtCore/QStringList>

// KDE includes
#include <KIconLoader>
#include <KLocale>
#include <KService>
#include <KServiceGroup>
#include <KDebug>

// QApt includes
#include <LibQApt/Backend>
#include <LibQApt/Config>

Application::Application(const QString &fileName, QApt::Backend *backend)
        : m_fileName(fileName)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(false)
        , m_isTechnical(false)
{
    m_data = desktopContents();
}

Application::Application(QApt::Package *package, QApt::Backend *backend)
        : m_backend(backend)
        , m_package(package)
        , m_isValid(true)
        , m_isTechnical(true)
{
}

Application::~Application()
{
}

QString Application::name()
{
    return i18n(untranslatedName().toUtf8());
}

QString Application::untranslatedName()
{
    QString name = QString::fromUtf8(getField("Name")).trimmed();
    if (name.isEmpty()) {
        // extras.ubuntu.com packages can have this
        name = package()->controlField(QLatin1String("Appname"));

        if (!name.isEmpty()) {
            // Graduate to non-technical, since it has Appname
            m_isTechnical = false;
            return name;
        }

        if (m_isTechnical) {
            return package()->shortDescription();
        } else {
            return package()->latin1Name();
        }
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
            if (m_isTechnical) {
                return package()->name();
            } else {
                return package()->shortDescription();
            }
        }
    }

    return i18n(comment.toUtf8());
}

QApt::Package *Application::package()
{
    if (!m_package) {
        QLatin1String packageName(getField("X-AppInstall-Package").data());
        if (m_backend) {
            m_package = m_backend->package(packageName);
        }
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

QString Application::menuPath()
{
    QString path;
    QString arrow(QString::fromUtf8(" ➜ "));

    // Take the file name and remove the .desktop ending
    QString desktopName = m_fileName.split('/').last().remove(QLatin1String(".desktop"));
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
        categories = package()->controlField(QLatin1String("Category"));

        if (!categories.isEmpty()) {
            // Graduate to non-technical, since it has Appname
            m_isTechnical = false;
        }
    }
    return categories;
}

KUrl Application::screenshotUrl(QApt::ScreenshotType type)
{
    QString appUrl;
    switch (type) {
    case QApt::Thumbnail:
        appUrl = package()->controlField(QLatin1String("Thumbnail-Url"));
        break;
    case QApt::Screenshot:
        appUrl = package()->controlField(QLatin1String("Screenshot-Url"));
        break;
    default:
        break;
    }

    KUrl url;
    if (appUrl.isEmpty()) {
        url = package()->screenshotUrl(type);
    } else {
        url = KUrl(appUrl);
    }

    return url;
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
        if (package->section().contains("lib")) {
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

QHash<QByteArray, QByteArray> Application::desktopContents()
{
    QHash<QByteArray, QByteArray> contents;

    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        return contents;
    }

    int lineIndex = 0;
    QByteArray buffer = file.readAll();

    QList<QByteArray> lines = buffer.split('\n');

    while (lineIndex < lines.size()) {
        QByteArray line = lines.at(lineIndex);
        if (line.isEmpty() || line.at(0) == '#') {
            lineIndex++;
            continue;
        }

        // Looking for a group heading.
        if (!m_isValid && line.startsWith('[') && line.contains(']')) {
            m_isValid = true;
        }

        QByteArray aKey;
        QByteArray aValue;
        int eqpos = line.indexOf('=');

        if (eqpos < 0) {
            // Invalid
            lineIndex++;
            continue;
        } else {
            aKey = line.left(eqpos);
            aValue = line.right(line.size() - eqpos -1);

            contents[aKey] = aValue;
        }

        lineIndex++;
    }

    return contents;
}
