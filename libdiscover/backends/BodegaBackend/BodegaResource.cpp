/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "BodegaResource.h"
#include "BodegaBackend.h"
#include <bodega/session.h>
#include <bodega/assetoperations.h>
#include <KLocalizedString>
#include <KGlobal>
#include <KLocalizedString>
#include <QDebug>

BodegaResource::BodegaResource(const Bodega::AssetInfo& info, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_info(info)
    , m_assetOperations(0)
{}

BodegaBackend* BodegaResource::backend() const
{
    return qobject_cast<BodegaBackend*>(parent());
}

QUrl BodegaResource::screenshotUrl()
{
    return m_info.images.value(Bodega::ImageLarge, 
                               m_info.images.value(Bodega::ImageHuge, 
                               m_info.images.value(Bodega::ImageMedium)));
}

QUrl BodegaResource::thumbnailUrl()
{
    return m_info.images.value(Bodega::ImageMedium);
}

AbstractResource::State BodegaResource::state()
{
    Bodega::AssetOperations* ops = assetOperations();
    if(!ops->isReady())
        return AbstractResource::Broken;
    return ops->isInstalled() ? AbstractResource::Installed : AbstractResource::None;
}

QString BodegaResource::icon() const
{
    return backend()->icon();
}

Bodega::AssetOperations* BodegaResource::assetOperations()
{
    if(!m_assetOperations) {
        Bodega::Session* session = backend()->session();
        m_assetOperations = session->assetOperations(m_info.id);
        connect(m_assetOperations, &Bodega::AssetOperations::installedChanged, this, &BodegaResource::stateChanged);
    }
    return m_assetOperations;
}

QStringList BodegaResource::categories()
{
    return QStringList(assetOperations()->assetTags().value("mimetype"));
}

int BodegaResource::size()
{
    //TODO
    return 0;
}

void BodegaResource::fetchChangelog()
{
    Bodega::ChangeLog log = assetOperations()->changeLog();
    QString description;
    
    for(QMap<QString, Bodega::ChangeLog::Entry>::const_iterator it=log.entries.constBegin(), itEnd=log.entries.constEnd();
        it!=itEnd; ++it)
    {
        description += i18nc("@info:label Refers to a software version, Ex: Version 1.2.1:", "Version %1:", it.key());

        QString issueDate = KGlobal::locale()->formatDateTime(QDateTime(it->timestamp), KLocale::ShortDate);
        description += QLatin1String("<p>") +
                       i18nc("@info:label", "This update was issued on %1", issueDate) +
                       QLatin1String("</p>");

        QString updateText = it->changes;
        updateText.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
        description += QLatin1String("<p><pre>") + updateText + QLatin1String("</pre></p>");
    }
    
    emit changelogFetched(description);
}
