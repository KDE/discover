/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

// QApt includes
#include <LibQApt/Backend>

Application::Application(const QString &fileName, QApt::Backend *backend)
        : m_fileName(fileName)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(false)
{
    m_data = desktopContents();
//     if (!m_isValid) {
//         kDebug() << "Not valid" << m_fileName;
//     }
}

Application::Application(QApt::Package *package)
        : m_package(package)
{
}

Application::~Application()
{
}

QString Application::name()
{
    QString name = QString::fromUtf8(getField("Name")).trimmed();
    if (name.isEmpty()) {
        // kDebug() << m_fileName;
        return package()->name();
    }

    return i18n(name.toUtf8());
}

QString Application::comment()
{
    QString comment = getField("Comment");
    if (comment.isEmpty()) {
        // Sometimes GenericName is used instead of Comment
        comment = getField("GenericName");
        if (comment.isEmpty()) {
            QApt::Package *pkg = package();

            if (pkg) {
                return pkg->shortDescription();
            }
        }
    }

    return i18n(comment.toUtf8());
}

QApt::Package *Application::package()
{
    if (!m_package) {
        QString packageName = getField("X-AppInstall-Package");
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

QString Application::icon()
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

    KService::Ptr service = KService::serviceByDesktopName(name());
    QVector<QPair<QString, QString> > ret;
    if (service) {
        ret = locateApplication(QString(), service->menuId());
    }
    if (!ret.isEmpty()) {
        path.append(QString("<img width=\"16\" heigh=\"16\"src=\"%1\"/>")
                    .arg(KIconLoader::global()->iconPath("kde", KIconLoader::Small)));
        path.append(QString("&nbsp;%1 <img width=\"16\" heigh=\"16\" src=\"%2\"/>&nbsp;%3")
                    .arg(QString::fromUtf8("➜"))
                    .arg(KIconLoader::global()->iconPath("applications-other", KIconLoader::Small))
                    .arg(i18n("Applications")));
        for (int i = 0; i < ret.size(); i++) {
            path.append(QString("&nbsp;%1&nbsp;<img width=\"16\" heigh=\"16\" src=\"%2\"/>&nbsp;%3")
                        .arg(QString::fromUtf8("➜"))
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

QList<QString> Application::categories()
{
    QString categoryString = getField("Categories");
    return categoryString.split(';');
}

int Application::popconScore()
{
    QString popconString = getField("X-AppInstall-Popcon");

    return popconString.toInt();
}

bool Application::isValid()
{
    return m_isValid;
}

QByteArray Application::getField(const QByteArray &field)
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
