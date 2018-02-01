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
#include <QStandardItemModel>
#include <KLocalizedString>
#include <KDesktopFile>
#include <PackageKit/Transaction>
#include <PackageKit/Daemon>
#include <QAction>
#include <QProcess>
#include <QDebug>
#include <resources/AbstractResourcesBackend.h>
#include "PackageKitBackend.h"
#include "config-paths.h"

class PKSourcesModel : public QStandardItemModel
{
public:
    PKSourcesModel(PackageKitSourcesBackend* backend)
        : QStandardItemModel(backend)
        , m_backend(backend) {}

    QHash<int, QByteArray> roleNames() const override
    {
        auto roles = QStandardItemModel::roleNames();
        roles[Qt::CheckStateRole] = "checked";
        return roles;
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role) override {
        auto item = itemFromIndex(index);
        if (!item)
            return false;

        switch(role) {
            case Qt::CheckStateRole: {
                auto transaction = PackageKit::Daemon::global()->repoEnable(item->data(AbstractSourcesBackend::IdRole).toString(), value.toInt() == Qt::Checked);
                connect(transaction, &PackageKit::Transaction::errorCode, m_backend, &PackageKitSourcesBackend::transactionError);
                return true;
            }
        }
        item->setData(value, role);
        return true;
    }

private:
    PackageKitSourcesBackend* m_backend;
};

static QAction* createActionForService(const QString &servicePath, QObject* parent)
{
    QAction* action = new QAction(parent);
    KDesktopFile parser(servicePath);
    action->setIcon(QIcon::fromTheme(parser.readIcon()));
    action->setText(parser.readName());
    QObject::connect(action, &QAction::triggered, action, [servicePath](){
        bool b = QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {servicePath});
        if (!b)
            qWarning() << "Could not start" << servicePath;
    });
    return action;
}

PackageKitSourcesBackend::PackageKitSourcesBackend(AbstractResourcesBackend* parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new PKSourcesModel(this))
{
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::repoListChanged, this, &PackageKitSourcesBackend::resetSources);
    resetSources();

    // Kubuntu-based
    auto service = PackageKitBackend::locateService(QStringLiteral("software-properties-kde.desktop"));
    if (!service.isEmpty())
        m_actions += createActionForService(service, this);

    // openSUSE-based
    service = PackageKitBackend::locateService(QStringLiteral("YaST2/sw_source.desktop"));
    if (!service.isEmpty())
        m_actions += createActionForService(service, this);
}

QString PackageKitSourcesBackend::idDescription()
{
    return i18n("Repository URL:");
}

QStandardItem* PackageKitSourcesBackend::findItemForId(const QString &id) const
{
    for(int i=0, c=m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(AbstractSourcesBackend::IdRole).toString() == id)
            return it;
    }
    return nullptr;
}

void PackageKitSourcesBackend::addRepositoryDetails(const QString &id, const QString &description, bool enabled)
{
    bool add = false;
    QStandardItem* item = findItemForId(id);

    if (!item) {
        item = new QStandardItem(id);
        add = true;
    }
    item->setData(id, IdRole);
    item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);

    if (add)
        m_sources->appendRow(item);
}

QAbstractItemModel * PackageKitSourcesBackend::sources()
{
    return m_sources;
}

bool PackageKitSourcesBackend::addSource(const QString& /*id*/)
{
    return false;
}

bool PackageKitSourcesBackend::removeSource(const QString& id)
{
    auto transaction = PackageKit::Daemon::global()->repoRemove(id, false);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    return false;
}

QList<QAction *> PackageKitSourcesBackend::actions() const
{
    return m_actions;
}

void PackageKitSourcesBackend::resetSources()
{
    m_sources->clear();
    auto transaction = PackageKit::Daemon::global()->getRepoList();
    connect(transaction, &PackageKit::Transaction::repoDetail, this, &PackageKitSourcesBackend::addRepositoryDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
}

void PackageKitSourcesBackend::transactionError(PackageKit::Transaction::Error error, const QString& message)
{
    Q_EMIT passiveMessage(message);
    qWarning() << "Transaction error: " << error << message << sender();
}
