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

    bool setData(const QModelIndex & index, const QVariant & value, int role) override
    {
        auto item = itemFromIndex(index);
        if(!item)
            return false;

        FwupdRemote* remote = fwupd_client_get_remote_by_id(m_backend->backend->client, item->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(), nullptr, nullptr);
        switch(role)
        {
            case Qt::CheckStateRole: {
                if(value == Qt::Checked) {
                    m_backend->m_currentItem = item;
                    if (fwupd_remote_get_approval_required(remote)) {
                        Q_EMIT m_backend->proceedRequest(i18n("Review EULA"), i18n("The remote %1 require that you accept their license:\n %2",
                                                    QString::fromUtf8(fwupd_remote_get_title(remote)),
                                                    QString::fromUtf8(fwupd_remote_get_agreement(remote))));
                    } else {
                        m_backend->proceed();
                    }
                } else if(value.toInt() == Qt::Unchecked) {
                    g_autoptr(GError) error = nullptr;
                    if(fwupd_client_modify_remote(m_backend->backend->client, fwupd_remote_get_id(remote), "Enabled", "false", nullptr, &error))
                        item->setCheckState(Qt::Unchecked);
                    else
                        qWarning() << "could not disable remote" << remote << error->message;
                }
                return true;
            }
        }
        return false;
    }

private:
    FwupdSourcesBackend* const m_backend;
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
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) remotes = fwupd_client_get_remotes(backend->client, nullptr, &error);
    if(!remotes) {
        qWarning() << "could not list fwupd remotes" << error->message;
        return;
    }

    for(uint i = 0; i < remotes->len; i++)
    {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
        if(fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL)
            continue;
        const QString id = QString::fromUtf8(fwupd_remote_get_id(remote));
        if(id.isEmpty())
            continue;

        QStandardItem* it = new QStandardItem(id);
        it->setData(id, AbstractSourcesBackend::IdRole);
        it->setData(QVariant(QString::fromUtf8(fwupd_remote_get_title(remote))), Qt::ToolTipRole);
        it->setCheckable(true);
        it->setCheckState(fwupd_remote_get_enabled(remote) ? Qt::Checked : Qt::Unchecked);
        m_sources->appendRow(it);
    }
}

QAbstractItemModel* FwupdSourcesBackend::sources()
{
    return m_sources;
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

QVariantList FwupdSourcesBackend::actions() const
{
    return {};
}

void FwupdSourcesBackend::cancel()
{
    FwupdRemote* remote = fwupd_client_get_remote_by_id(backend->client, m_currentItem->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),nullptr,nullptr);
    m_currentItem->setCheckState(fwupd_remote_get_enabled(remote) ? Qt::Checked : Qt::Unchecked);

    m_currentItem = nullptr;
}

void FwupdSourcesBackend::proceed()
{
    FwupdRemote* remote = fwupd_client_get_remote_by_id(backend->client, m_currentItem->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),nullptr,nullptr);
    g_autoptr(GError) error = nullptr;
    if (fwupd_client_modify_remote(backend->client, fwupd_remote_get_id(remote), "Enabled", "true", nullptr, &error))
        m_currentItem->setData(Qt::Checked, Qt::CheckStateRole);
    else
        qWarning() << "could not enable remote" << remote << error->message;

    m_currentItem = nullptr;
}

#include "FwupdSourcesBackend.moc"
