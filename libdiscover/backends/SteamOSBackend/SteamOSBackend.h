/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef STEAMOSBACKEND_H
#define STEAMOSBACKEND_H

#include <QDBusMessage>
#include <QPointer>
#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

class ComSteampoweredAtomupd1Interface;
class QDBusPendingCallWatcher;
class StandardBackendUpdater;
class SteamOSResource;
class SteamOSTransaction;

class SteamOSBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit SteamOSBackend(QObject *parent = nullptr);

    // Some constants from com.steampowered.Atomupd1 dbus interface
    enum Status {
        Idle = 0,
        InProgress,
        Paused,
        Successful,
        Failed,
        Cancelled,
    };

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    QHash<QString, SteamOSResource *> resources() const;
    bool isValid() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;

    void checkForUpdates() override;
    QString displayName() const override;
    int fetchingUpdatesProgress() const override
    {
        return m_fetching > 0 ? 42 : 100;
    }

    static QString service();
    static QString path();

public Q_SLOTS:
    void checkForUpdatesFinished(QDBusPendingCallWatcher *call);

private Q_SLOTS:
    void needRebootChanged();

private:
    void hasUpdateChanged(bool hasUpdate);

    void acquireFetching(bool f);

    /* Check if a transaction exists already */
    bool fetchExistingTransaction();

    /* Helper to setup a Transaction and connect all signals/slots */
    void setupTransaction(SteamOSResource *app);

    QHash<QString, SteamOSResource *> m_resources;
    StandardBackendUpdater *m_updater;
    uint m_fetching = 0;

    QString m_updateVersion; // Next update version, can use once we get from dbus.
    QString m_updateBuildID; // Next build version.
    quint64 m_updateSize; // Estimated size of next update

    QPointer<SteamOSResource> m_resource; // Since we only ever have one, cache it.

    /* The current transaction in progress, if any */
    SteamOSTransaction *m_transaction;

    QPointer<ComSteampoweredAtomupd1Interface> m_interface; // Interface to atomupd dbus api
    QString m_currentVersion;
    QString m_currentBuildID;
};

#endif // STEAMOSBACKEND_H
