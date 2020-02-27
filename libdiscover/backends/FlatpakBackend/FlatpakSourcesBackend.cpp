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
#include <KSharedConfig>
#include <KConfigGroup>
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
    , m_noSourcesItem(new QStandardItem(QStringLiteral("-")))
{
    m_flathubAction->setToolTip(QStringLiteral("flathub"));
    connect(m_flathubAction, &QAction::triggered, this, [this](){
        addSource(QStringLiteral("https://flathub.org/repo/flathub.flatpakrepo"));
    });
    for (auto installation : installations) {
        if (!listRepositories(installation)) {
            qWarning() << "Failed to list repositories from installation" << installation;
        }
    }

    m_noSourcesItem->setEnabled(false);
    if (m_sources->rowCount() == 0) {
        m_sources->appendRow(m_noSourcesItem);
    }
}

FlatpakSourcesBackend::~FlatpakSourcesBackend()
{
    QStringList ids;
    for (int i = 0, c = m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        ids << it->data(IdRole).toString();
    }

    auto conf = KSharedConfig::openConfig();
    KConfigGroup group = conf->group("FlatpakSources");
    group.writeEntry("Sources", ids);
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

    auto addSource = [=](AbstractResource* res) {
        if (res)
            backend->installApplication(res);
        else
            Q_EMIT backend->passiveMessage(i18n("Could not add the source %1", flatpakrepoUrl.toDisplayString()));
    };

    if (flatpakrepoUrl.isLocalFile()) {
        addSource(backend->addSourceFromFlatpakRepo(flatpakrepoUrl));
    } else {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = flatpakrepoUrl;
        auto stream = new StoredResultsStream ({backend->search(filter)});
        connect(stream, &StoredResultsStream::finished, this, [addSource, stream]() {
            const auto res = stream->resources();
            addSource(res.value(0));
        });
    }
    return true;
}

QStandardItem * FlatpakSourcesBackend::sourceById(const QString& id) const
{
    QStandardItem* sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(IdRole) == id) {
            sourceIt = it;
            break;
        }
    }
    return sourceIt;
}

QStandardItem * FlatpakSourcesBackend::sourceByUrl(const QString& _url) const
{
    QUrl url(_url);

    QStandardItem* sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i<c && !sourceIt; ++i) {
        auto it = m_sources->item(i);
        if (url.matches(it->data(Qt::StatusTipRole).toUrl(), QUrl::StripTrailingSlash)) {
            sourceIt = it;
            break;
        }
    }
    return sourceIt;
}

bool FlatpakSourcesBackend::removeSource(const QString &id)
{
    auto sourceIt = sourceById(id);
    if (sourceIt) {
        FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem*>(sourceIt);
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) error = nullptr;
        const auto installation = sourceItem->flatpakInstallation();

        g_autoptr(GPtrArray) refs = flatpak_installation_list_remote_refs_sync(installation, id.toUtf8().constData(), cancellable, &error);
        QHash<QString, QStringList> toRemoveHash;
        toRemoveHash.reserve(refs->len);
        QStringList toRemoveRefs;
        toRemoveRefs.reserve(refs->len);
        FlatpakBackend* backend = qobject_cast<FlatpakBackend*>(parent());
        for (uint i = 0; i < refs->len; i++) {
            FlatpakRef *ref= FLATPAK_REF(g_ptr_array_index(refs, i));

            g_autoptr(GError) error = nullptr;
            FlatpakInstalledRef* installedRef = flatpak_installation_get_installed_ref(installation, flatpak_ref_get_kind(ref), flatpak_ref_get_name(ref), flatpak_ref_get_arch(ref), flatpak_ref_get_branch(ref), cancellable, &error);
            if (installedRef) {
                auto res = backend->getAppForInstalledRef(installation, installedRef);
                const auto name = QString::fromUtf8(flatpak_ref_get_name(ref));
                const auto refString = QString::fromUtf8(flatpak_ref_format_ref(ref));
                if (!name.endsWith(QLatin1String(".Locale"))) {
                    if (res)
                        toRemoveHash[res->name()] << refString;
                    else
                        toRemoveHash[refString] << refString;
                }
                toRemoveRefs << refString;
            }
        }
        QStringList toRemove;
        toRemove.reserve(toRemoveHash.count());
        for (auto it = toRemoveHash.constBegin(), itEnd = toRemoveHash.constEnd(); it != itEnd; ++it) {
            if (it.value().count() > 1)
                toRemove << QStringLiteral("%1 - %2").arg(it.key(), it.value().join(QLatin1String(", ")));
            else
                toRemove << it.key();
        }
        toRemove.sort();

        if (!toRemove.isEmpty()) {
            m_proceedFunctions.push([this, toRemoveRefs, installation, id] {
                g_autoptr(GError) localError = nullptr;
                g_autoptr(GCancellable) cancellable = g_cancellable_new();
                g_autoptr(FlatpakTransaction) transaction = flatpak_transaction_new_for_installation(installation, cancellable, &localError);
                for (const QString& instRef : qAsConst(toRemoveRefs)) {
                    const QByteArray refString = instRef.toUtf8();
                    flatpak_transaction_add_uninstall(transaction, refString.constData(), &localError);
                    if (localError)
                        return;
                }

                if (flatpak_transaction_run(transaction, cancellable, &localError)) {
                    removeSource(id);
                }
            });


            Q_EMIT proceedRequest(i18n("Removing '%1'", id), i18n("To remove this repository, the following applications must be uninstalled:<ul><li>%1</li></ul>", toRemove.join(QStringLiteral("</li><li>"))));
            return false;
        }

        if (flatpak_installation_remove_remote(installation, id.toUtf8().constData(), cancellable, &error)) {
            m_sources->removeRow(sourceItem->row());

            if (m_sources->rowCount() == 0) {
                m_sources->appendRow(m_noSourcesItem);
            }
            return true;
        } else {
            Q_EMIT passiveMessage(i18n("Failed to remove %1 remote repository: %2", id, QString::fromUtf8(error->message)));
            return false;
        }
    } else {
        Q_EMIT passiveMessage(i18n("Could not find %1", id));
        return false;
    }

    return false;
}

QVariantList FlatpakSourcesBackend::actions() const
{
    return { QVariant::fromValue<QObject*>(m_flathubAction) };
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
        qWarning() << "Source " << resource->flatpakName() << " already exists in" << flatpak_installation_get_path(m_preferredInstallation);
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

    const auto theActions = actions();
    for(const QVariant& act: theActions) {
        QAction* action = qobject_cast<QAction*>(act.value<QObject*>());
        if (action->toolTip() == id) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    FlatpakSourceItem *it = new FlatpakSourceItem(!title.isEmpty() ? title : id);
    it->setData(remoteUrl.isLocalFile() ? remoteUrl.toLocalFile() : remoteUrl.host(), Qt::ToolTipRole);
    it->setData(remoteUrl, Qt::StatusTipRole);
    it->setData(id, IdRole);
    it->setData(QString::fromUtf8(flatpak_remote_get_icon(remote)), IconUrlRole);
    it->setFlatpakInstallation(installation);

    int idx = -1;
    {
        const auto conf = KSharedConfig::openConfig();
        const KConfigGroup group = conf->group("FlatpakSources");
        const auto ids = group.readEntry<QStringList>("Sources", QStringList());

        const auto ourIdx = ids.indexOf(id);
        if (ourIdx<0) { //If not present, we put it on top
            idx = 0;
        } else {
            idx=0;
            for(int c=m_sources->rowCount(); idx<c; ++idx) {
                const auto compIt = m_sources->item(idx);
                const int compIdx = ids.indexOf(compIt->data(IdRole).toString());
                if (compIdx >= ourIdx) {
                    break;
                }
            }
        }
    }

    m_sources->insertRow(idx, it);
    if (m_sources->rowCount() == 1)
        Q_EMIT firstSourceIdChanged();
    Q_EMIT lastSourceIdChanged();

    if (m_sources->rowCount() > 0) {
        m_sources->takeRow(m_noSourcesItem->row());
    }
}

QString FlatpakSourcesBackend::idDescription()
{
    return i18n("Flatpak repository URI (*.flatpakrepo)");
}

bool FlatpakSourcesBackend::moveSource(const QString& sourceId, int delta)
{
    auto item = sourceById(sourceId);
    if (!item)
        return false;
    const auto row = item->row();
    auto prevRow = m_sources->takeRow(row);
    Q_ASSERT(!prevRow.isEmpty());

    const auto destRow = row + (delta>0? delta : delta);
    m_sources->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0)
        Q_EMIT firstSourceIdChanged();
    if (destRow == m_sources->rowCount() - 1 || row == m_sources->rowCount() - 1)
        Q_EMIT lastSourceIdChanged();
    return true;
}

int FlatpakSourcesBackend::originIndex(const QString& sourceId) const
{
    auto item = sourceById(sourceId);
    return item ? item->row() : INT_MAX;
}

void FlatpakSourcesBackend::cancel()
{
    m_proceedFunctions.pop();
}

void FlatpakSourcesBackend::proceed()
{
    m_proceedFunctions.pop()();
}
