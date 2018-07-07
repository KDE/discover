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

#include <QDebug>
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
        if (!item)
            return false;

        remote = fwupd_client_get_remote_by_id(m_backend->backend->client,item->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),NULL,NULL);
        status = fwupd_remote_get_enabled(remote);
        switch(role)
        {
            case Qt::CheckStateRole:
            {
                if((value.toInt() == Qt::Checked) )
                {
                     m_backend->eulaRequired(QLatin1String(fwupd_remote_get_title(remote)),QLatin1String(fwupd_remote_get_agreement(remote)));
                     connect(m_backend,&FwupdSourcesBackend::proceed,this,
                             [=]()
                                {
                                 if((value.toInt() == Qt::Checked) )
                                 {
                                    if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("true")).toUtf8().constData()),NULL,NULL))
                                        item->setData(value, role);
                                 }
                                 else
                                 {
                                     if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("false")).toUtf8().constData()),NULL,NULL))
                                        item->setData(value, role);
                                 }
                                }
                             );
                }
                else if(value.toInt() == Qt::Unchecked)
                {
                    if((value.toInt() == Qt::Checked) )
                    {
                       if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("true")).toUtf8().constData()),NULL,NULL))
                           item->setData(value, role);
                    }
                    else
                    {
                        if(fwupd_client_modify_remote(m_backend->backend->client,fwupd_remote_get_id(remote),QString(QLatin1String("Enabled")).toUtf8().constData(),(QString(QLatin1String("false")).toUtf8().constData()),NULL,NULL))
                           item->setData(value, role);
                    }
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
    , m_sources(new FwupdSourcesModel(this))
{
    backend = qobject_cast<FwupdBackend*>(parent);
    g_autoptr(GPtrArray) remotes = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    /* find all remotes */
    remotes = fwupd_client_get_remotes (backend->client,cancellable,&error);
    if(remotes != NULL)
     {
        for (int i = 0; i < (int)remotes->len; i++)
        {
            FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index (remotes, i);
            if (fwupd_remote_get_kind (remote) == FWUPD_REMOTE_KIND_LOCAL)
                continue;
            addSource(QLatin1String(fwupd_remote_get_id (remote)));
        }
      }
}

QAbstractItemModel* FwupdSourcesBackend::sources()
{
    return m_sources;
}

void FwupdSourcesBackend::eulaRequired( const QString& remoteName , const QString& licenseAgreement)
{
    Q_EMIT proceedRequest(i18n("Accept EULA"), i18n("The remote %1 require that you accept their license:\n %2",
                                                 remoteName, licenseAgreement));
}

void FwupdSourcesBackend::cancel()
{
    qDebug() << "Request Cancelled";
}

bool FwupdSourcesBackend::addSource(const QString& id)
{   
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    FwupdBackend* backend = qobject_cast<FwupdBackend*>(parent());
    FwupdRemote* remote;
    bool status ;

    if (id.isEmpty())
        return false;
    
    remote = fwupd_client_get_remote_by_id(backend->client,id.toUtf8().constData(),cancellable,&error);
    status = !fwupd_remote_get_enabled(remote);

    QStandardItem* it = new QStandardItem(id);
    it->setData(id, AbstractSourcesBackend::IdRole);
    it->setData(QVariant(QLatin1Literal(fwupd_remote_get_title (remote))), Qt::ToolTipRole);
    it->setCheckable(true);
    it->setCheckState(status ? Qt::Unchecked : Qt::Checked);
    m_sources->appendRow(it);
    return true;
}

bool FwupdSourcesBackend::removeSource(const QString& id)
{
    qWarning() << "Removal of Sources Not Allowed" << "Remote-ID" << id;
    return false;
}

QList<QAction*> FwupdSourcesBackend::actions() const
{
    return  m_actions ;
}

#include "FwupdSourcesBackend.moc"


