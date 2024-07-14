/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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
    void handleKAuthUpgradeHelperProgress(KJob *job, unsigned long percent);

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

    bool m_progressing = false;
    qreal m_upgradeProgress = 0.0;
};


#endif // ALPINEAPKUPDATER_H
