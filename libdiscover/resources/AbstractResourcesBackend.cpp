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

#include "AbstractResourcesBackend.h"
#include <QHash>
#include <QStandardPaths>
#include <QDebug>

AbstractResourcesBackend::AbstractResourcesBackend(QObject* parent)
    : QObject(parent)
{
}

void AbstractResourcesBackend::installApplication(AbstractResource* app)
{
    installApplication(app, AddonList());
}

void AbstractResourcesBackend::integrateActions(KActionCollection*)
{}

void AbstractResourcesBackend::setMetaData(const QString&)
{}

void AbstractResourcesBackend::setName(const QString& name)
{
    m_name = name;
}

QString AbstractResourcesBackend::name() const
{
    return m_name;
}

QString AbstractResourcesBackend::categoriesFilePath() const
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("libdiscover/categories/")+name()+QStringLiteral("-categories.xml"));
    if (path.isEmpty()) {
        qWarning() << "Couldn't find a category for " << name();
    }
    return path;
}
