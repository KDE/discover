/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#include "FwupdResource.h"
#include "FwupdBackend.h"

#include <Transaction/AddonList.h>
#include <krandom.h>
#include <QDesktopServices>
#include <QStringList>
#include <QTimer>

Q_GLOBAL_STATIC_WITH_ARGS(QVector<QString>, s_icons, ({ QLatin1String("kdevelop"), QLatin1String("kalgebra"), QLatin1String("kmail"), QLatin1String("akregator"), QLatin1String("korganizer") }))

FwupdResource::FwupdResource(QString name, bool isTechnical, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_name(std::move(name))
    , m_state(State::Broken)
    , m_iconName((*s_icons)[KRandom::random() % s_icons->size()])
    , m_addons({ PackageState(QStringLiteral("a"), QStringLiteral("aaaaaa"), false), PackageState(QStringLiteral("b"), QStringLiteral("aaaaaa"), false), PackageState(QStringLiteral("c"), QStringLiteral("aaaaaa"), false)})
    , m_isTechnical(isTechnical)
{
    const int nofScreenshots = KRandom::random() % 5;
    m_screenshots = QList<QUrl>{
        QUrl(QStringLiteral("http://screenshots.debian.net/screenshots/000/014/863/large.png")),
        QUrl(QStringLiteral("https://c2.staticflickr.com/6/5656/21772158034_dc84382527_o.jpg")),
        QUrl(QStringLiteral("https://c1.staticflickr.com/9/8479/8166397343_b78106f353_k.jpg")),
        QUrl(QStringLiteral("https://c2.staticflickr.com/4/3685/9954407993_dad10a6943_k.jpg")),
        QUrl(QStringLiteral("https://c1.staticflickr.com/1/653/22527103378_8ce572e1de_k.jpg"))
    }.mid(nofScreenshots);
    m_screenshotThumbnails = m_screenshots;
}

QList<PackageState> FwupdResource::addonsInformation()
{
    return m_addons;
}

QString FwupdResource::availableVersion() const
{
    return m_version;
}

QStringList FwupdResource::categories()
{
   return m_categories;
}

void FwupdResource::addCategories(const QString &category){
    m_categories.append(category);
}

QString FwupdResource::comment()
{
    return m_summary;
}

int FwupdResource::size()
{
    return m_size;
}

QUrl FwupdResource::homepage()
{
    return m_homepage;
}

QUrl FwupdResource::helpURL()
{
    return QUrl(QStringLiteral("http://very-very-excellent-docs.lol"));
}

QUrl FwupdResource::bugURL()
{
    return QUrl(QStringLiteral("file:///dev/null"));
}

QUrl FwupdResource::donationURL()
{
    return QUrl(QStringLiteral("https://youtu.be/0o8XMlL8rqY"));
}

QVariant FwupdResource::icon() const
{
    return isTechnical() ? QStringLiteral("kalarm") : m_iconName;
}

QString FwupdResource::installedVersion() const
{
    return m_version;
}

QString FwupdResource::license()
{
    return m_license;
}

QString FwupdResource::longDescription()
{
    return m_description;
}

QString FwupdResource::name() const
{
    return m_name;
}

QString FwupdResource::vendor() const
{
    return m_vendor;
}

QString FwupdResource::origin() const
{
    return QStringLiteral("FwupdSource1");
}

QString FwupdResource::packageName() const
{
    return m_name;
}

QString FwupdResource::section()
{
    return QStringLiteral("fwupd");
}

AbstractResource::State FwupdResource::state()
{
    return m_state;
}

void FwupdResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    emit changelogFetched(log);
}

void FwupdResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(m_screenshotThumbnails, m_screenshots);
}

void FwupdResource::setState(AbstractResource::State state)
{
    m_state = state;
    emit stateChanged();
}

void FwupdResource::setAddons(const AddonList& addons)
{
    Q_FOREACH (const QString& toInstall, addons.addonsToInstall()) {
        setAddonInstalled(toInstall, true);
    }
    Q_FOREACH (const QString& toRemove, addons.addonsToRemove()) {
        setAddonInstalled(toRemove, false);
    }
}

void FwupdResource::setAddonInstalled(const QString& addon, bool installed)
{
    for(auto & elem : m_addons) {
        if(elem.name() == addon) {
            elem.setInstalled(installed);
        }
    }
}


void FwupdResource::invokeApplication() const
{
    QDesktopServices d;
    d.openUrl(QUrl(QStringLiteral("https://projects.kde.org/projects/extragear/sysadmin/muon")));
}

QUrl FwupdResource::url() const
{
    return QUrl(QLatin1String("fwupd://") + packageName().replace(QLatin1Char(' '), QLatin1Char('.')));
}
