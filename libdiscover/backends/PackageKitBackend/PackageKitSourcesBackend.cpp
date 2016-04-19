/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PackageKitSourcesBackend.h"
#include <KLocalizedString>
#include <PackageKit/Transaction>
#include <PackageKit/Daemon>
#include <QDebug>

PackageKitSourcesBackend::PackageKitSourcesBackend(QObject* parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new QStandardItemModel(this))
{
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::repoListChanged, this, &PackageKitSourcesBackend::resetSources);
    resetSources();
}

QString PackageKitSourcesBackend::name() const
{
    return i18n("Software Management");
}

QString PackageKitSourcesBackend::idDescription()
{
    return i18n("Repository description:");
}

QStandardItem* PackageKitSourcesBackend::findItemForId(const QString &id) const
{
    for(int i=0, c=m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        if (it->text() == id)
            return it;
    }
    return nullptr;
}

void PackageKitSourcesBackend::addRepositoryDetails(const QString &id, const QString &description, bool enabled)
{
    QStandardItem* item = findItemForId(id);

    if (!item) {
        item = new QStandardItem;
        item->setText(id);
        m_sources->appendRow(item);
    }
    item->setData(description, Qt::ToolTipRole);
    item->setEnabled(enabled);
}

QAbstractItemModel * PackageKitSourcesBackend::sources()
{
    return m_sources;
}

bool PackageKitSourcesBackend::addSource(const QString& id)
{
    auto transaction = PackageKit::Daemon::global()->repoEnable(id, true);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    return true;
}

bool PackageKitSourcesBackend::removeSource(const QString& id)
{
    auto transaction = PackageKit::Daemon::global()->repoRemove(id, false);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    return false;
}

QList<QAction *> PackageKitSourcesBackend::actions() const
{
    return {};
}

void PackageKitSourcesBackend::resetSources()
{
    m_sources->clear();
    auto transaction = PackageKit::Daemon::global()->getRepoList();
    connect(transaction, &PackageKit::Transaction::repoDetail, this, &PackageKitSourcesBackend::addRepositoryDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
}

void PackageKitSourcesBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
}
