/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RPMOSTREE1BACKEND_H
#define RPMOSTREE1BACKEND_H

#include "RpmOstreeDBusInterface.h"
#include "RpmOstreeResource.h"
#include "RpmOstreeTransaction.h"

#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

class RpmOstreeBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit RpmOstreeBackend(QObject *parent = nullptr);

    /*
     * Executing "rpm-ostree update --check" using QProcess to check if
     * there is a new deployment version avaliable.
     */
    void executeCheckUpdateProcess();

    /*
     * Executing "ostree remote refs kinoite" using QProcess to get
     * a list of the avaliable remote refs list.
     */
    void filterRemoteRefs();

    /* List refs for a given remote */
    QStringList getRemoteRefs(const QString &);

    /*
     * Getting the output resulting from executing the QProcess remote refs list.
     */
    void readRefsOutput(QIODevice *device);

    /*
     * Calling Rebase method from the rpm-ostree DBus class when
     * there is a new kinoite refs.
     */
    void rebaseToNewVersion(QString);

    /* Convenience function to set the fetching status and emit the corresponding signal */
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
    void toggleFetching();

private:
    /* Get the currently booted deployment */
    RpmOstreeResource *currentlyBootedDeployment(void);

    /* The list of available deployments */
    QVector<RpmOstreeResource *> m_resources;
    /* The current transaction in progress, if any */
    RpmOstreeTransaction *m_transaction;
    /* DBus path to the currently booted OS DBus interface */
    QString m_bootedObjectPath;

    StandardBackendUpdater *m_updater;
    bool m_fetching;
};

#endif
