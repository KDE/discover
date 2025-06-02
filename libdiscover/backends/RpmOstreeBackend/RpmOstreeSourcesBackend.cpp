/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeSourcesBackend.h"

#include <KLocalizedString>
#include <QDebug>

#include <ostree-repo.h>
#include <ostree.h>

#include "libdiscover_rpm-ostree_debug.h"

RpmOstreeSourcesBackend::RpmOstreeSourcesBackend(AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_model(new QStandardItemModel(this))
{
    g_autoptr(GFile) path = g_file_new_for_path("/ostree/repo");
    g_autoptr(OstreeRepo) repo = ostree_repo_new(path);
    if (repo == NULL) {
        qCInfo(RPMOSTREE_LOG) << "Could not find ostree repo:" << path;
        return;
    }

    g_autoptr(GError) err = NULL;
    gboolean res = ostree_repo_open(repo, NULL, &err);
    if (!res) {
        qCInfo(RPMOSTREE_LOG) << "Could not open ostree repo:" << path;
        return;
    }

    guint remote_count = 0;
    char **remotes = ostree_repo_remote_list(repo, &remote_count);
    for (guint r = 0; r < remote_count; ++r) {
        auto remote = new QStandardItem(QString::fromUtf8(remotes[r]));

        char *url = NULL;
        res = ostree_repo_remote_get_url(repo, remotes[r], &url, &err);
        if (res) {
            remote->setData(QString::fromUtf8(url), Qt::ToolTipRole);
            free(url);
        } else {
            qCWarning(RPMOSTREE_LOG) << "Could not get the URL for ostree remote:" << remotes[r];
        }

        m_model->appendRow(remote);
    }

    for (guint r = 0; r < remote_count; ++r) {
        free(remotes[r]);
    }
    free(remotes);
}

QAbstractItemModel *RpmOstreeSourcesBackend::sources()
{
    return m_model;
}

bool RpmOstreeSourcesBackend::addSource(const QString &)
{
    return false;
}

bool RpmOstreeSourcesBackend::removeSource(const QString &)
{
    return false;
}

QString RpmOstreeSourcesBackend::idDescription()
{
    return i18n("ostree remotes");
}

QVariantList RpmOstreeSourcesBackend::actions() const
{
    return {};
}

bool RpmOstreeSourcesBackend::supportsAdding() const
{
    return false;
}

bool RpmOstreeSourcesBackend::canMoveSources() const
{
    return false;
}

#include "moc_RpmOstreeSourcesBackend.cpp"
