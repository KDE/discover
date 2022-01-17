/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakSourcesBackend.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QStandardPaths>
#include <QTemporaryFile>
#include <glib.h>
#include <resources/DiscoverAction.h>
#include <resources/StoredResultsStream.h>

class FlatpakSourceItem : public QStandardItem
{
public:
    FlatpakSourceItem(const QString &text, FlatpakRemote *remote, FlatpakBackend *backend)
        : QStandardItem(text)
        , m_remote(remote)
        , m_backend(backend)
    {
        g_object_ref(remote);
    }
    ~FlatpakSourceItem()
    {
        g_object_unref(m_remote);
    }

    void setFlatpakInstallation(FlatpakInstallation *installation)
    {
        m_installation = installation;
    }

    FlatpakInstallation *flatpakInstallation() const
    {
        return m_installation;
    }

    void setData(const QVariant &value, int role) override
    {
        // We check isCheckable() so the initial setting of the item doesn't trigger a change
        if (role == Qt::CheckStateRole && isCheckable()) {
            const bool disabled = flatpak_remote_get_disabled(m_remote);
            const bool requestedDisabled = Qt::Unchecked == value;
            if (disabled != requestedDisabled) {
                flatpak_remote_set_disabled(m_remote, requestedDisabled);
                g_autoptr(GError) error = nullptr;
                if (!flatpak_installation_modify_remote(m_installation, m_remote, nullptr, &error)) {
                    qWarning() << "set disabled failed" << error->message;
                    return;
                }

                if (requestedDisabled) {
                    m_backend->unloadRemote(m_installation, m_remote);
                } else {
                    m_backend->loadRemote(m_installation, m_remote);
                }
            }
        }
        QStandardItem::setData(value, role);
    }

private:
    FlatpakInstallation *m_installation = nullptr;
    FlatpakRemote *const m_remote;
    FlatpakBackend *const m_backend;
};

FlatpakSourcesBackend::FlatpakSourcesBackend(const QVector<FlatpakInstallation *> &installations, AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_preferredInstallation(installations.constFirst())
    , m_sources(new QStandardItemModel(this))
    , m_flathubAction(new DiscoverAction("flatpak-discover", i18n("Add Flathub"), this))
    , m_noSourcesItem(new QStandardItem(QStringLiteral("-")))
{
    m_flathubAction->setObjectName(QStringLiteral("flathub"));
    m_flathubAction->setToolTip(i18n("Makes it possible to easily install the applications listed in https://flathub.org"));
    connect(m_flathubAction, &DiscoverAction::triggered, this, [this]() {
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
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto it = m_sources->item(i);
        ids << it->data(IdRole).toString();
    }

    auto conf = KSharedConfig::openConfig();
    KConfigGroup group = conf->group("FlatpakSources");
    group.writeEntry("Sources", ids);

    if (!m_noSourcesItem->model())
        delete m_noSourcesItem;
}

QAbstractItemModel *FlatpakSourcesBackend::sources()
{
    return m_sources;
}

bool FlatpakSourcesBackend::addSource(const QString &id)
{
    FlatpakBackend *backend = qobject_cast<FlatpakBackend *>(parent());
    const QUrl flatpakrepoUrl(id);

    if (id.isEmpty() || !flatpakrepoUrl.isValid())
        return false;

    auto addSource = [=](AbstractResource *res) {
        if (res)
            backend->installApplication(res);
        else
            Q_EMIT backend->passiveMessage(i18n("Could not add the source %1", flatpakrepoUrl.toDisplayString()));
    };

    if (flatpakrepoUrl.isLocalFile()) {
        auto stream = new ResultsStream(QStringLiteral("FlatpakSource-") + flatpakrepoUrl.toDisplayString());
        backend->addSourceFromFlatpakRepo(flatpakrepoUrl, stream);
        connect(stream, &ResultsStream::resourcesFound, this, [addSource](const QVector<AbstractResource *> &res) {
            addSource(res.constFirst());
        });
    } else {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = flatpakrepoUrl;
        auto stream = new StoredResultsStream({backend->search(filter)});
        connect(stream, &StoredResultsStream::finished, this, [addSource, stream]() {
            const auto res = stream->resources();
            addSource(res.value(0));
        });
    }
    return true;
}

QStandardItem *FlatpakSourcesBackend::sourceById(const QString &id) const
{
    QStandardItem *sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(IdRole) == id) {
            sourceIt = it;
            break;
        }
    }
    return sourceIt;
}

QStandardItem *FlatpakSourcesBackend::sourceByUrl(const QString &_url) const
{
    QUrl url(_url);

    QStandardItem *sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i < c && !sourceIt; ++i) {
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
        FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem *>(sourceIt);
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) error = nullptr;
        const auto installation = sourceItem->flatpakInstallation();

        g_autoptr(GPtrArray) refs = flatpak_installation_list_remote_refs_sync(installation, id.toUtf8().constData(), cancellable, &error);
        if (refs) {
            QHash<QString, QStringList> toRemoveHash;
            toRemoveHash.reserve(refs->len);
            QStringList toRemoveRefs;
            toRemoveRefs.reserve(refs->len);
            FlatpakBackend *backend = qobject_cast<FlatpakBackend *>(parent());
            for (uint i = 0; i < refs->len; i++) {
                FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));

                g_autoptr(GError) error = nullptr;
                FlatpakInstalledRef *installedRef = flatpak_installation_get_installed_ref(installation,
                                                                                           flatpak_ref_get_kind(ref),
                                                                                           flatpak_ref_get_name(ref),
                                                                                           flatpak_ref_get_arch(ref),
                                                                                           flatpak_ref_get_branch(ref),
                                                                                           cancellable,
                                                                                           &error);
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
                    for (const QString &instRef : qAsConst(toRemoveRefs)) {
                        const QByteArray refString = instRef.toUtf8();
                        flatpak_transaction_add_uninstall(transaction, refString.constData(), &localError);
                        if (localError)
                            return;
                    }

                    if (flatpak_transaction_run(transaction, cancellable, &localError)) {
                        removeSource(id);
                    }
                });

                Q_EMIT proceedRequest(i18n("Removing '%1'", id),
                                      i18n("To remove this repository, the following applications must be uninstalled:<ul><li>%1</li></ul>",
                                           toRemove.join(QStringLiteral("</li><li>"))));
                return false;
            }
        } else {
            qWarning() << "could not list refs in repo" << id << error->message;
        }

        g_autoptr(GError) errorRemoveRemote = nullptr;
        if (flatpak_installation_remove_remote(installation, id.toUtf8().constData(), cancellable, &errorRemoveRemote)) {
            m_sources->removeRow(sourceItem->row());

            if (m_sources->rowCount() == 0) {
                m_sources->appendRow(m_noSourcesItem);
            }
            return true;
        } else {
            Q_EMIT passiveMessage(i18n("Failed to remove %1 remote repository: %2", id, QString::fromUtf8(errorRemoveRemote->message)));
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
    return {QVariant::fromValue<QObject *>(m_flathubAction)};
}

bool FlatpakSourcesBackend::listRepositories(FlatpakInstallation *installation)
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

void FlatpakSourcesBackend::populateRemote(FlatpakRemote *remote, const QString &name, const QString &url, const QString &gpgKey)
{
    flatpak_remote_set_url(remote, url.toUtf8().constData());
    flatpak_remote_set_noenumerate(remote, false);
    flatpak_remote_set_title(remote, name.toUtf8().constData());

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
}

FlatpakRemote *FlatpakSourcesBackend::installSource(FlatpakResource *resource)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();

    auto remote = flatpak_installation_get_remote_by_name(m_preferredInstallation, resource->flatpakName().toUtf8().constData(), cancellable, nullptr);
    if (remote) {
        qWarning() << "Source " << resource->flatpakName() << " already exists in" << flatpak_installation_get_path(m_preferredInstallation);
        return nullptr;
    }

    remote = flatpak_remote_new(resource->flatpakName().toUtf8().constData());
    populateRemote(remote,
                   resource->comment(),
                   resource->getMetadata(QStringLiteral("repo-url")).toString(),
                   resource->getMetadata(QStringLiteral("gpg-key")).toString());
    if (!resource->branch().isEmpty()) {
        flatpak_remote_set_default_branch(remote, resource->branch().toUtf8().constData());
    }

    g_autoptr(GError) error = nullptr;
    if (!flatpak_installation_modify_remote(m_preferredInstallation, remote, cancellable, &error)) {
        qWarning() << "Failed to add source " << resource->flatpakName() << error->message;
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
    for (const QVariant &act : theActions) {
        DiscoverAction *action = qobject_cast<DiscoverAction *>(act.value<QObject *>());
        if (action->objectName() == id) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    QString label = !title.isEmpty() ? title : id;
    if (flatpak_installation_get_is_user(installation)) {
        label = i18n("%1 (user)", label);
    }

    FlatpakBackend *backend = qobject_cast<FlatpakBackend *>(parent());
    FlatpakSourceItem *it = new FlatpakSourceItem(label, remote, backend);
    it->setData(remoteUrl.isLocalFile() ? remoteUrl.toLocalFile() : remoteUrl.host(), Qt::ToolTipRole);
    it->setData(remoteUrl, Qt::StatusTipRole);
    it->setData(id, IdRole);
    it->setCheckState(flatpak_remote_get_disabled(remote) ? Qt::Unchecked : Qt::Checked);
#if FLATPAK_CHECK_VERSION(1, 4, 0)
    it->setData(QString::fromUtf8(flatpak_remote_get_icon(remote)), IconUrlRole);
#endif
    it->setCheckable(true);
    it->setFlatpakInstallation(installation);

    int idx = -1;
    {
        const auto conf = KSharedConfig::openConfig();
        const KConfigGroup group = conf->group("FlatpakSources");
        const auto ids = group.readEntry<QStringList>("Sources", QStringList());

        const auto ourIdx = ids.indexOf(id);
        if (ourIdx < 0) { // If not present, we put it on top
            idx = 0;
        } else {
            idx = 0;
            for (int c = m_sources->rowCount(); idx < c; ++idx) {
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
    return i18n("Enter a Flatpak repository URI (*.flatpakrepo):");
}

bool FlatpakSourcesBackend::moveSource(const QString &sourceId, int delta)
{
    auto item = sourceById(sourceId);
    if (!item)
        return false;
    const auto row = item->row();
    auto prevRow = m_sources->takeRow(row);
    Q_ASSERT(!prevRow.isEmpty());

    const auto destRow = row + (delta > 0 ? delta : delta);
    m_sources->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0)
        Q_EMIT firstSourceIdChanged();
    if (destRow == m_sources->rowCount() - 1 || row == m_sources->rowCount() - 1)
        Q_EMIT lastSourceIdChanged();
    return true;
}

int FlatpakSourcesBackend::originIndex(const QString &sourceId) const
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

void FlatpakSourcesBackend::checkRepositories(const QStringList &repoNames)
{
    FlatpakBackend *backend = qobject_cast<FlatpakBackend *>(parent());
    const auto insts = backend->installations();
    for (const QString &repoName : repoNames) {
        const QByteArray name = repoName.toUtf8();
        for (auto installation : insts) {
            g_autoptr(GError) error = nullptr;
            auto remote = flatpak_installation_get_remote_by_name(installation, name, nullptr, &error);
            if (remote) {
                addRemote(remote, installation);
            }
        }
    }
}
