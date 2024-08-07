/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "resources/DiscoverAction.h"

#include "AlpineApkAuthActionFactory.h"
#include "AlpineApkSourcesBackend.h"
#include "alpineapk_backend_logging.h" // generated by ECM

#include <QAction>
#include <QDebug>
#include <QVector>

// KF5
#include <KAuth/ExecuteJob>
#include <KLocalizedString>

// libapk-qt
#include <QtApk>

AlpineApkSourcesBackend::AlpineApkSourcesBackend(AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_sourcesModel(new QStandardItemModel(this))
    , m_refreshAction(new DiscoverAction(QStringLiteral("view-refresh"), QStringLiteral("Reload"), this))
    , m_saveAction(new DiscoverAction(QStringLiteral("document-save"), QStringLiteral("Save"), this))
{
    loadSources();
    QObject::connect(m_refreshAction, &DiscoverAction::triggered, this, &AlpineApkSourcesBackend::loadSources);
    QObject::connect(m_saveAction, &DiscoverAction::triggered, this, &AlpineApkSourcesBackend::saveSources);
    // track enabling/disabling repo source
    QObject::connect(m_sourcesModel, &QStandardItemModel::itemChanged, this, &AlpineApkSourcesBackend::onItemChanged);
}

QAbstractItemModel *AlpineApkSourcesBackend::sources()
{
    return m_sourcesModel;
}

QStandardItem *AlpineApkSourcesBackend::sourceForId(const QString &id) const
{
    for (int i = 0; i < m_sourcesModel->rowCount(); ++i) {
        QStandardItem *item = m_sourcesModel->item(i, 0);
        if (item->data(AbstractSourcesBackend::IdRole) == id) {
            return item;
        }
    }
    return nullptr;
}

bool AlpineApkSourcesBackend::addSource(const QString &id)
{
    m_repos.append(QtApk::Repository(id, QString(), true));
    fillModelFromRepos();
    return true;
}

void AlpineApkSourcesBackend::loadSources()
{
    m_repos = QtApk::Database::getRepositories();
    fillModelFromRepos();
}

void AlpineApkSourcesBackend::fillModelFromRepos()
{
    m_sourcesModel->clear();
    for (const QtApk::Repository &repo : m_repos) {
        if (repo.url.isEmpty()) {
            continue;
        }
        qCDebug(LOG_ALPINEAPK) << "source backend: Adding source:" << repo.url << repo.enabled;
        QStandardItem *it = new QStandardItem(repo.url);
        it->setData(repo.url, AbstractSourcesBackend::IdRole);
        it->setData(repo.comment, Qt::ToolTipRole);
        it->setCheckable(true);
        it->setCheckState(repo.enabled ? Qt::Checked : Qt::Unchecked);
        m_sourcesModel->appendRow(it);
    }
}

void AlpineApkSourcesBackend::saveSources()
{
    const QVariant repoUrls = QVariant::fromValue<QVector<QtApk::Repository>>(m_repos);

    // run with elevated privileges
    KAuth::ExecuteJob *reply = ActionFactory::createRepoconfigAction(repoUrls);
    if (!reply)
        return;

    QObject::connect(reply, &KAuth::ExecuteJob::result, this, [this](KJob *job) {
        KAuth::ExecuteJob *reply = static_cast<KAuth::ExecuteJob *>(job);
        if (reply->error() != 0) {
            const QString errMessage = reply->errorString();
            qCWarning(LOG_ALPINEAPK) << "KAuth helper returned error:" << reply->error() << errMessage;
            if (reply->error() == KAuth::ActionReply::Error::AuthorizationDeniedError) {
                Q_EMIT passiveMessage(i18n("Authorization denied"));
            } else {
                Q_EMIT passiveMessage(i18n("Error: ") + errMessage);
            }
        }
        this->loadSources();
    });

    reply->start();
}

void AlpineApkSourcesBackend::onItemChanged(QStandardItem *item)
{
    // update internal storage vector and reload model from it
    //    otherwise checks state are not updated in UI
    const Qt::CheckState cs = item->checkState();
    const QModelIndex idx = m_sourcesModel->indexFromItem(item);
    m_repos[idx.row()].enabled = (cs == Qt::Checked);
    fillModelFromRepos();
}

bool AlpineApkSourcesBackend::removeSource(const QString &id)
{
    const QStandardItem *it = sourceForId(id);
    if (!it) {
        qCWarning(LOG_ALPINEAPK) << "source backend: couldn't find " << id;
        return false;
    }
    m_repos.remove(it->row());
    return m_sourcesModel->removeRow(it->row());
}

QString AlpineApkSourcesBackend::idDescription()
{
    return i18nc("Adding repo",
                 "Enter Alpine repository URL, for example: "
                 "http://dl-cdn.alpinelinux.org/alpine/edge/testing/");
}

QVariantList AlpineApkSourcesBackend::actions() const
{
    static const QVariantList s_actions{
        QVariant::fromValue<QObject *>(m_saveAction),
        QVariant::fromValue<QObject *>(m_refreshAction),
    };
    return s_actions;
}

bool AlpineApkSourcesBackend::supportsAdding() const
{
    return true;
}

bool AlpineApkSourcesBackend::canMoveSources() const
{
    return true;
}

bool AlpineApkSourcesBackend::moveSource(const QString &sourceId, int delta)
{
    int row = sourceForId(sourceId)->row();
    QList<QStandardItem *> prevRow = m_sourcesModel->takeRow(row);
    if (prevRow.isEmpty()) {
        return false;
    }

    const int destRow = row + delta;
    m_sourcesModel->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0) {
        Q_EMIT firstSourceIdChanged();
    }
    if (destRow == (m_sourcesModel->rowCount() - 1) || row == (m_sourcesModel->rowCount() - 1)) {
        Q_EMIT lastSourceIdChanged();
    }

    // swap also items in internal storage vector
    m_repos.swapItemsAt(row, destRow);

    return true;
}
