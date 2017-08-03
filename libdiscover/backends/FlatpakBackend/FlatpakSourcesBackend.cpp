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

#include <glib.h>
#include <QTemporaryFile>
#include <QStandardPaths>

class FlatpakSourceItem : public QStandardItem
{
public:
    FlatpakSourceItem(const QString &text) : QStandardItem(text) { }
    void setFlatpakInstallation(FlatpakInstallation *installation) { m_installation = installation; }
    FlatpakInstallation *flatpakInstallation() const { return m_installation; }

private:
    FlatpakInstallation *m_installation;
};

FlatpakSourcesBackend::FlatpakSourcesBackend(const QVector<FlatpakInstallation *> &installations, QObject* parent)
    : AbstractSourcesBackend(parent)
    , m_preferredInstallation(installations.constFirst())
    , m_sources(new QStandardItemModel(this))
{
    QHash<int, QByteArray> roles = m_sources->roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    roles.insert(Qt::UserRole, "flatpakInstallation");
    m_sources->setItemRoleNames(roles);

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

static void addRepo(FlatpakBackend* backend, const QUrl &fileUrl)
{
    Q_ASSERT(backend);
    auto res = backend->addSourceFromFlatpakRepo(fileUrl);
    if (!res) {
        qWarning() << "Couldn't add" << fileUrl;
        return;
    }
    backend->installApplication(res);
}

bool FlatpakSourcesBackend::addSource(const QString &id)
{
    FlatpakBackend* backend = qobject_cast<FlatpakBackend*>(parent());

    const QUrl flatpakrepoUrl = QUrl::fromUserInput(id);
    if (flatpakrepoUrl.isLocalFile()) {
        addRepo(backend, flatpakrepoUrl);
    } else {
        const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1Char('/') + flatpakrepoUrl.fileName());

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        auto replyGet = manager->get(QNetworkRequest(flatpakrepoUrl));

        connect(replyGet, &QNetworkReply::finished, this, [this, manager, replyGet, fileUrl, backend] {
            if (replyGet->error() != QNetworkReply::NoError)
                return;

            auto replyPut = manager->put(QNetworkRequest(fileUrl), replyGet->readAll());
            connect(replyPut, &QNetworkReply::finished, this, [fileUrl, backend, manager]() {
                addRepo(backend, fileUrl);
                delete manager;
            });
        });
    }
    return true;
}

bool FlatpakSourcesBackend::removeSource(const QString &id)
{
    QList<QStandardItem*> items = m_sources->findItems(id);
    if (items.count() == 1) {
        FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem*>(items.first());
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        if (flatpak_installation_remove_remote(sourceItem->flatpakInstallation(), id.toUtf8().constData(), cancellable, nullptr)) {
            m_sources->removeRow(sourceItem->row());
            return true;
        } else {
            qWarning() << "Failed to remove " << id << " remote repository";
            return false;
        }
    } else {
        qWarning() << "couldn't find " << id  << items;
        return false;
    }

    return false;
}

QList<QAction*> FlatpakSourcesBackend::actions() const
{
    return {};
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

        const QString id = QString::fromUtf8(flatpak_remote_get_name(remote));
        const QString title = QString::fromUtf8(flatpak_remote_get_title(remote));

        FlatpakSourceItem *it = new FlatpakSourceItem(id);
        it->setCheckState(flatpak_remote_get_disabled(remote) ? Qt::Unchecked : Qt::Checked);
        it->setData(title.isEmpty() ? id : title, Qt::ToolTipRole);
        it->setData(name(), AbstractSourcesBackend::SectionRole);
        it->setData(QVariant::fromValue<QObject*>(this), AbstractSourcesBackend::SourcesBackend);
        it->setFlatpakInstallation(installation);

        m_sources->appendRow(it);
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

    FlatpakSourceItem *it = new FlatpakSourceItem(resource->flatpakName());
    it->setCheckState(flatpak_remote_get_disabled(remote) ? Qt::Unchecked : Qt::Checked);
    it->setData(resource->comment().isEmpty() ? resource->flatpakName() : resource->comment(), Qt::ToolTipRole);
    it->setData(name(), AbstractSourcesBackend::SectionRole);
    it->setData(QVariant::fromValue<QObject*>(this), AbstractSourcesBackend::SourcesBackend);
    it->setFlatpakInstallation(m_preferredInstallation);

    m_sources->appendRow(it);

    return remote;
}

QString FlatpakSourcesBackend::idDescription()
{
    return i18n("Flatpak repository URI (*.flatpakrepo)");
}
