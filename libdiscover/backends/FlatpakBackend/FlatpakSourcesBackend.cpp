/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "FlatpakSourcesBackend.h"
#include "FlatpakResource.h"
#include "FlatpakBackend.h"
#include <KLocalizedString>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAction>

#include <glib.h>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <resources/StoredResultsStream.h>

class FlatpakSourceItem : public QStandardItem
{
public:
    FlatpakSourceItem(const QString &text) : QStandardItem(text) { }
    void setFlatpakInstallation(FlatpakInstallation *installation) { m_installation = installation; }
    FlatpakInstallation *flatpakInstallation() const { return m_installation; }

private:
    FlatpakInstallation *m_installation;
};

FlatpakSourcesBackend::FlatpakSourcesBackend(const QVector<FlatpakInstallation *> &installations, AbstractResourcesBackend * parent)
    : AbstractSourcesBackend(parent)
    , m_preferredInstallation(installations.constFirst())
    , m_sources(new QStandardItemModel(this))
    , m_flathubAction(new QAction(i18n("Add Flathub"), this))
{
    QHash<int, QByteArray> roles = m_sources->roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    roles.insert(Qt::UserRole, "flatpakInstallation");
    m_sources->setItemRoleNames(roles);

    m_flathubAction->setToolTip(QStringLiteral("flathub"));
    connect(m_flathubAction, &QAction::triggered, this, [this](){
        addSource(QStringLiteral("https://flathub.org/repo/flathub.flatpakrepo"));
    });
    for (auto installation : installations) {
        if (!listRepositories(installation)) {
            qWarning() << "Failed to list repositories from installation" << installation;
        }
    }

}

QAbstractItemModel* FlatpakSourcesBackend::sources()
{
    return m_sources;
}

bool FlatpakSourcesBackend::addSource(const QString &id)
{
    FlatpakBackend* backend = qobject_cast<FlatpakBackend*>(parent());
    const QUrl flatpakrepoUrl(id);

    if (id.isEmpty() || !flatpakrepoUrl.isValid())
        return false;

    if (flatpakrepoUrl.isLocalFile()) {
        auto res = backend->addSourceFromFlatpakRepo(flatpakrepoUrl);
        if (res)
            backend->installApplication(res);
        else
            backend->passiveMessage(i18n("Could not add the source %1", flatpakrepoUrl.toDisplayString()));
    } else {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = flatpakrepoUrl;
        auto stream = new StoredResultsStream ({backend->search(filter)});
        connect(stream, &StoredResultsStream::finished, this, [backend, stream, flatpakrepoUrl]() {
            const auto res = stream->resources();
            if (!res.isEmpty()) {
                Q_ASSERT(res.count() == 1);
                backend->installApplication(res.first());
            } else {
                backend->passiveMessage(i18n("Could not add the source %1", flatpakrepoUrl.toDisplayString()));
            }
        });
    }
    return true;
}

bool FlatpakSourcesBackend::removeSource(const QString &id)
{
    QStandardItem* sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(IdRole) == id) {
            sourceIt = it;
            break;
        }
    }

    if (sourceIt) {
        FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem*>(sourceIt);
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) error = NULL;
        if (flatpak_installation_remove_remote(sourceItem->flatpakInstallation(), id.toUtf8().constData(), cancellable, &error)) {
            m_sources->removeRow(sourceItem->row());
            return true;
        } else {
            qWarning() << "Failed to remove " << id << " remote repository:" << error->message;
            return false;
        }
    } else {
        qWarning() << "couldn't find " << id;
        return false;
    }

    return false;
}

QList<QAction*> FlatpakSourcesBackend::actions() const
{
    return { m_flathubAction };
}

bool FlatpakSourcesBackend::listRepositories(FlatpakInstallation* installation)
{
    Q_ASSERT(installation);

    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GPtrArray) remotes = flatpak_installation_list_remotes(installation, cancellable, nullptr);

    if (!remotes) {
        return false;
    }

    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));

        if (flatpak_remote_get_noenumerate(remote)) {
            continue;
        }

        addRemote(remote, installation);
    }

    return true;
}

FlatpakRemote * FlatpakSourcesBackend::installSource(FlatpakResource *resource)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();

    auto remote = flatpak_installation_get_remote_by_name(m_preferredInstallation, resource->flatpakName().toUtf8().constData(), cancellable, nullptr);
    if (remote) {
        qWarning() << "Source " << resource->flatpakName() << " already exists";
        return nullptr;
    }

    remote = flatpak_remote_new(resource->flatpakName().toUtf8().constData());
    flatpak_remote_set_url(remote, resource->getMetadata(QStringLiteral("repo-url")).toString().toUtf8().constData());
    flatpak_remote_set_noenumerate(remote, false);
    flatpak_remote_set_title(remote, resource->comment().toUtf8().constData());

    const QString gpgKey = resource->getMetadata(QStringLiteral("gpg-key")).toString();
    if (!gpgKey.isEmpty()) {
        gsize dataLen = 0;
        g_autofree guchar *data = nullptr;
        g_autoptr(GBytes) bytes = nullptr;
        data = g_base64_decode(gpgKey.toUtf8().constData(), &dataLen);
        bytes = g_bytes_new(data, dataLen);
        flatpak_remote_set_gpg_verify(remote, true);
        flatpak_remote_set_gpg_key(remote, bytes);
    } else {
        flatpak_remote_set_gpg_verify(remote, false);
    }

    if (!resource->branch().isEmpty()) {
        flatpak_remote_set_default_branch(remote, resource->branch().toUtf8().constData());
    }

    if (!flatpak_installation_modify_remote(m_preferredInstallation, remote, cancellable, nullptr)) {
        qWarning() << "Failed to add source " << resource->flatpakName();
        return nullptr;
    }

    addRemote(remote, m_preferredInstallation);

    return remote;
}

void FlatpakSourcesBackend::addRemote(FlatpakRemote *remote, FlatpakInstallation *installation)
{
    const QString id = QString::fromUtf8(flatpak_remote_get_name(remote));
    const QString title = QString::fromUtf8(flatpak_remote_get_title(remote));
    const QUrl remoteUrl(QString::fromUtf8(flatpak_remote_get_url(remote)));

    for(QAction *action: actions()) {
        if (action->toolTip() == id) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    FlatpakSourceItem *it = new FlatpakSourceItem(!title.isEmpty() ? title : id);
    it->setCheckState(flatpak_remote_get_disabled(remote) ? Qt::Unchecked : Qt::Checked);
    it->setData(remoteUrl.host(), Qt::ToolTipRole);
    it->setData(id, IdRole);
    it->setFlatpakInstallation(installation);

    m_sources->appendRow(it);
}

QString FlatpakSourcesBackend::idDescription()
{
    return i18n("Flatpak repository URI (*.flatpakrepo)");
}
