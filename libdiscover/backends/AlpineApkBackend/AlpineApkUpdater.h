/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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

#ifndef ALPINEAPKUPDATER_H
#define ALPINEAPKUPDATER_H

#include "resources/AbstractBackendUpdater.h"
#include "resources/AbstractResourcesBackend.h"

#include <QSet>
#include <QDateTime>
#include <QTimer>
#include <QVariant>
#include <QMap>
#include <QVector>

#include <QtApkChangeset.h>

class AbstractResourcesBackend;
class AlpineApkBackend;
class KJob;
namespace KAuth {
    class ExecuteJob;
}

class AlpineApkUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)

public:
    explicit AlpineApkUpdater(AbstractResourcesBackend *parent = nullptr);

    /**
     * This method is called, when Muon switches to the updates view.
     * Here the backend should mark all upgradeable packages as to be upgraded.
     */
    void prepare() override;

    /**
     * @returns true if the backend contains packages which can be updated
     */
    bool hasUpdates() const override;
    /**
     * @returns the progress of the update in percent
     */
    qreal progress() const override;

    /**
     * This method is used to remove resources from the list of packages
     * marked to be upgraded. It will potentially be called before \start.
     */
    void removeResources(const QList<AbstractResource*> &apps) override;

    /**
     * This method is used to add resource to the list of packages marked to be upgraded.
     * It will potentially be called before \start.
     */
    void addResources(const QList<AbstractResource*> &apps) override;

    /**
     * @returns the list of updateable resources in the system
     */
    QList<AbstractResource *> toUpdate() const override;

    /**
     * @returns the QDateTime when the last update happened
     */
    QDateTime lastUpdate() const override;

    /**
     * @returns whether the updater can currently be canceled or not
     * @see cancelableChanged
     */
    bool isCancelable() const override;

    /**
     * @returns whether the updater is currently running or not
     * this property decides, if there will be progress reporting in the GUI.
     * This has to stay true during the whole transaction!
     * @see progressingChanged
     */
    bool isProgressing() const override;

    /**
     * @returns whether @p res is marked for update
     */
    bool isMarked(AbstractResource* res) const override;

    void fetchChangelog() const override;

    /**
     * @returns the size of all the packages set to update combined
     */
    double updateSize() const override;

    /**
     * @returns the speed at which we are downloading
     */
    quint64 downloadSpeed() const override;

public Q_SLOTS:
    /**
     * If \isCancelable is true during the transaction, this method has
     * to be implemented and will potentially be called when the user
     * wants to cancel the update.
     */
    void cancel() override;

    /**
     * This method starts the update. All packages which are in \toUpdate
     * are going to be updated.
     *
     * From this moment on the AbstractBackendUpdater should continuously update
     * the other methods to show its progress.
     *
     * @see progress
     * @see progressChanged
     * @see isProgressing
     * @see progressingChanged
     */
    void start() override;

    /**
     * Answers a proceed request
     */
    void proceed() override;

Q_SIGNALS:
    void checkForUpdatesFinished();
    void updatesCountChanged(int updatesCount);
    void fetchingUpdatesProgressChanged(int progress);
    //void cancelTransaction();

public Q_SLOTS:
    int updatesCount();
    void startCheckForUpdates();

    // KAuth handler slots
    // update
    void handleKAuthUpdateHelperReply(KJob *job);
    void handleKAuthUpdateHelperProgress(KJob *job, unsigned long percent);
    // upgrade
    void handleKAuthUpgradeHelperReply(KJob *job);

    //void transactionRemoved(Transaction* t);
    //void cleanup();

public:
    QVector<QtApk::ChangesetItem> &changes() { return m_upgradeable.changes(); }
    const QVector<QtApk::ChangesetItem> &changes() const { return m_upgradeable.changes(); }

protected:
    void handleKAuthHelperError(KAuth::ExecuteJob *reply, const QVariantMap &replyData);

private:
    AlpineApkBackend *const m_backend;
    int m_updatesCount = 0;
    QtApk::Changeset m_upgradeable;
    QSet<AbstractResource *> m_allUpdateable;
    QSet<AbstractResource *> m_markedToUpdate;
//    void resourcesChanged(AbstractResource* res, const QVector<QByteArray>& props);
//    void refreshUpdateable();
//    void transactionAdded(Transaction* newTransaction);
//    void transactionProgressChanged();
//    void refreshProgress();
//    QVector<Transaction*> transactions() const;

//    QSet<AbstractResource*> m_upgradeable;
//    QSet<AbstractResource*> m_pendingResources;
//    bool m_settingUp;
//    qreal m_progress;
//    QDateTime m_lastUpdate;
//    QTimer m_timer;
//    bool m_canCancel = false;
};


#endif // ALPINEAPKUPDATER_H
