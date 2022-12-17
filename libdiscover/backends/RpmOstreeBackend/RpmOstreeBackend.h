/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "RpmOstreeDBusInterface.h"
#include "RpmOstreeResource.h"
#include "RpmOstreeTransaction.h"

#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

#include <AppStreamQt/pool.h>
#include <QTimer>

class DiscoverAction;

/*
 * This backend currently uses a mix of DBus calls and direct `rpm-ostree`
 * command line calls to operate. This is de to the fact that support for direct
 * Peer to Peer DBus connections (used for all rpm-ostree transactions) appear
 * to not work properly with Qt for an unknown reason.
 *
 * TODO: Replace code calling the command line by calls via the DBus interface
 */
class RpmOstreeBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit RpmOstreeBackend(QObject *parent = nullptr);

    /* Convenience function to set the fetching status and emit the
     * corresponding signal */
    void setFetching(bool);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;

    /* Returns true if we are running on an ostree/rpm-ostree managed system */
    bool isValid() const override;

    Transaction *installApplication(AbstractResource *) override;
    Transaction *installApplication(AbstractResource *, const AddonList &) override;
    Transaction *removeApplication(AbstractResource *) override;

    bool isFetching() const override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;

public Q_SLOTS:
    /* Fetch or refresh the list of deployments */
    void refreshDeployments();

    /* Look for a new Major version and offer it as an update */
    void lookForNextMajorVersion();

    /* Rebase to the next major version */
    void rebaseToNewVersion();

private:
    /* Once rpm-ostree has effectively stated, registrer ourselves as update
     * driver to the rpm-ostreed daemon to make sure that it does not exit while
     * we are running and then initialize the rest of the backend. */
    void initializeBackend();

    /* Called when a new major version is found to setup user facing actions */
    void foundNewMajorVersion(const QString &newMajorVersion);

    /* Set to true once we've successfully registrered with rpm-ostree */
    bool m_registrered;

    /* The list of available deployments */
    QVector<RpmOstreeResource *> m_resources;

    /* The currently booted deployment */
    RpmOstreeResource *m_currentlyBootedDeployment;

    /* The current transaction in progress, if any */
    RpmOstreeTransaction *m_transaction;

    /* DBus path to the currently booted OS DBus interface */
    QString m_bootedObjectPath;

    /* Watcher for rpm-ostree DBus service  */
    QDBusServiceWatcher *m_watcher;

    /* Timer for DBus retries */
    QTimer *m_dbusActivationTimer;

    /* Qt bindings to the main rpm-ostree DBus interface */
    OrgProjectatomicRpmostree1SysrootInterface *m_interface;

    /* We're re-using the standard backend updater logic */
    StandardBackendUpdater *m_updater;

    /* Used when refreshing the list of deployments */
    bool m_fetching;

    /* AppStream pool to be able to the distribution major release versions */
    QScopedPointer<AppStream::Pool> m_appdata;

    /* Enable development branches and releases. This is mostly usuelful for
     * testing and should not be enabled by default. We might add an hidden
     * option in the future.
     * Behaviour for Fedora: Enable rebasing to Rawhide */
    bool m_developmentEnabled;

    /* Custom message that takes into account major upgrades. To display when:
     * - A new major version is available
     * - An update to the current version is available or pending a reboot */
    QSharedPointer<InlineMessage> m_rebootBeforeRebaseMessage;

    /* Custom message that takes into account major upgrades. To display when:
     * - A new major version is available
     * - No update to the current version are available or pending a reboot */
    QSharedPointer<InlineMessage> m_rebaseAvailableMessage;
};
