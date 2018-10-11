/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "FwupdSourcesBackend.h"

#include <QAction>
#include <QString>
#include <KLocalizedString>


class FwupdSourcesModel : public QStandardItemModel
{
Q_OBJECT
public:
    FwupdSourcesModel(FwupdSourcesBackend* backend)
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
        if(!item)
            return false;
        remote = fwupd_client_get_remote_by_id(m_backend->backend->client, item->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),nullptr,nullptr);
        status = fwupd_remote_get_enabled(remote);
        switch(role)
        {
            case Qt::CheckStateRole:
            {
                if((value.toInt() == Qt::Checked) )
                {
                    #if FWUPD_CHECK_VERSION(1,0,7)
                        m_backend->eulaRequired(QString::fromUtf8(fwupd_remote_get_title(remote)),QString::fromUtf8(fwupd_remote_get_agreement(remote)));
                    #endif
                    connect(m_backend,&FwupdSourcesBackend::proceed,this,
                        [=]()
                            {
                                if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("true")).toUtf8().constData()),nullptr,nullptr))
                                    item->setData(value, role);
                            }
                            );
                    connect(m_backend,&FwupdSourcesBackend::cancel,this,
                        [=]()
                            {
                                item->setCheckState(Qt::Unchecked);
                                Q_EMIT dataChanged(index,index,{});
                                return false;
                            }
                            );
                }
                else if(value.toInt() == Qt::Unchecked)
                {
                    if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("false")).toUtf8().constData()),nullptr,nullptr))
                           item->setData(value, role);
                }

            }
        }
        Q_EMIT dataChanged(index,index,{});
        return true;
    }

private:
    FwupdSourcesBackend* m_backend;
    FwupdRemote* remote;
    bool status;
};

FwupdSourcesBackend::FwupdSourcesBackend(AbstractResourcesBackend * parent)
    : AbstractSourcesBackend(parent)
    , backend(qobject_cast<FwupdBackend*>(parent))
    , m_sources(new FwupdSourcesModel(this))
{
    populateSources();
}

void FwupdSourcesBackend::populateSources()
{
    /* find all remotes */
    g_autoptr(GPtrArray) remotes = fwupd_client_get_remotes(backend->client,nullptr,nullptr);
    if(remotes != nullptr)
    {
        for(uint i = 0; i < remotes->len; i++)
        {
            FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
            if(fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL)
                continue;
            const QString id = QString::fromUtf8(fwupd_remote_get_id(remote));
            if(id.isEmpty())
                continue;
            bool status = !fwupd_remote_get_enabled(remote);
            QStandardItem* it = new QStandardItem(id);
            it->setData(id, AbstractSourcesBackend::IdRole);
            it->setData(QVariant(QString::fromUtf8(fwupd_remote_get_title(remote))), Qt::ToolTipRole);
            it->setCheckable(true);
            it->setCheckState(status ? Qt::Unchecked : Qt::Checked);
            m_sources->appendRow(it);
        }
    }
}

QAbstractItemModel* FwupdSourcesBackend::sources()
{
    return m_sources;
}

void FwupdSourcesBackend::eulaRequired( const QString& remoteName, const QString& licenseAgreement)
{
    Q_EMIT proceedRequest(i18n("Accept EULA"), i18n("The remote %1 require that you accept their license:\n %2",
                                                 remoteName, licenseAgreement));
}

bool FwupdSourcesBackend::addSource(const QString& id)
{   
    qWarning() << "Fwupd Error: Custom Addition of Sources Not Allowed" << "Remote-ID" << id;
    return false;
}

bool FwupdSourcesBackend::removeSource(const QString& id)
{
    qWarning() << "Fwupd Error: Removal of Sources Not Allowed" << "Remote-ID" << id;
    return false;
}

QList<QAction*> FwupdSourcesBackend::actions() const
{
    return  {} ;
}

#include "FwupdSourcesBackend.moc"


