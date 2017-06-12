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
#include "AbstractResource.h"
#include "Category/Category.h"
#include <QHash>
#include <QMetaObject>
#include <QMetaProperty>
#include <QDebug>
#include <QTimer>

QDebug operator<<(QDebug debug, const AbstractResourcesBackend::Filters& filters)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Filters(";
    if (filters.category) debug.nospace() << "category: " << filters.category << ',';
    if (filters.state) debug.nospace() << "state: " << filters.state << ',';
    if (!filters.mimetype.isEmpty()) debug.nospace() << "mimetype: " << filters.mimetype << ',';
    if (!filters.search.isEmpty()) debug.nospace() << "search: " << filters.search << ',';
    if (!filters.extends.isEmpty()) debug.nospace() << "extends:" << filters.extends << ',';
    if (!filters.roles.isEmpty()) debug.nospace() << "roles:" << filters.roles << ',';
    debug.nospace() << ')';

    return debug;
}

ResultsStream::ResultsStream(const QString &objectName, const QVector<AbstractResource*>& resources)
    : ResultsStream(objectName)
{
    Q_ASSERT(!resources.contains(nullptr));
    QTimer::singleShot(0, this, [resources, this] () {
        if (!resources.isEmpty())
            Q_EMIT resourcesFound(resources);
        finish();
    });
}

ResultsStream::ResultsStream(const QString &objectName)
{
    setObjectName(objectName);
    QTimer::singleShot(5000, this, [objectName]() { qDebug() << "stream took really long" << objectName; });
}

ResultsStream::~ResultsStream()
{
}

void ResultsStream::finish()
{
    deleteLater();
}

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

void AbstractResourcesBackend::setName(const QString& name)
{
    m_name = name;
}

QString AbstractResourcesBackend::name() const
{
    return m_name;
}

void AbstractResourcesBackend::emitRatingsReady()
{
    emit allDataChanged({ "rating", "ratingPoints", "ratingCount", "sortableRating" });
}

bool AbstractResourcesBackend::Filters::shouldFilter(AbstractResource* res) const
{
    Q_ASSERT(res);

    if(!extends.isEmpty() && !res->extends().contains(extends)) {
        return false;
    }
    if(!resourceUrl.isEmpty() && res->url() != resourceUrl) {
        return false;
    }

    for(QHash<QByteArray, QVariant>::const_iterator it=roles.constBegin(), itEnd=roles.constEnd(); it!=itEnd; ++it) {
        Q_ASSERT(AbstractResource::staticMetaObject.indexOfProperty(it.key().constData())>=0);
        if(res->property(it.key().constData()) != it.value()) {
            return false;
        }
    }

    if(res->state() < state)
        return false;

    if(!mimetype.isEmpty() && !res->mimetypes().contains(mimetype)) {
        return false;
    }

    return !category || res->categoryMatches(category);
}

void AbstractResourcesBackend::Filters::filterJustInCase(QVector<AbstractResource *>& input) const
{
    for(auto it = input.begin(); it != input.end();) {
        if (shouldFilter(*it))
            ++it;
        else
            it = input.erase(it);
    }
}

QStringList AbstractResourcesBackend::extends() const
{
    return {};
}
