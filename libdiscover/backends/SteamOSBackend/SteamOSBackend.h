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
class SteamOSBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit SteamOSBackend(QObject *parent = nullptr);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    QHash<QString, SteamOSResource *> resources() const;
    bool isValid() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;

    bool isFetching() const override;
    void checkForUpdates() override;
    QString displayName() const override;

    static QString service();
    static QString path();

public Q_SLOTS:
    void checkForUpdatesFinished(QDBusPendingCallWatcher *call);

private Q_SLOTS:
    void needRebootChanged();

private:
    void hasUpdateChanged(bool hasUpdate);

    void acquireFetching(bool f);

    QHash<QString, SteamOSResource *> m_resources;
    StandardBackendUpdater *m_updater;
    uint m_fetching = 0;

    QString m_updateVersion; // Next update version, can use once we get from dbus.
    QString m_updateBuild; // Next build version.
    quint64 m_updateSize; // Estimated size of next update

    QPointer<SteamOSResource> m_resource; // Since we only ever have one, cache it.

    QPointer<ComSteampoweredAtomupd1Interface> m_interface; // Interface to atomupd dbus api
    QString m_currentVersion;
};

#endif // STEAMOSBACKEND_H
