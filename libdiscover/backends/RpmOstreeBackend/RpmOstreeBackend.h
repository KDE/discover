/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RPMOSTREE1BACKEND_H
#define RPMOSTREE1BACKEND_H

#include <resources/AbstractResourcesBackend.h>

#include "rpmostree1.h"
#include <QMetaType>
#include <QProcess>
#include <QSharedPointer>
#include <QThreadPool>
#include <QtDBus/QDBusPendingReply>

#include <AppStreamQt/component.h>
#include <appstream/AppStreamIntegration.h>
#include <appstream/OdrsReviewsBackend.h>

class RpmOstreeReviewsBackend;
class OdrsReviewsBackend;
class RpmOstreeResource;
class StandardBackendUpdater;
class RpmOstreeBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit RpmOstreeBackend(QObject *parent = nullptr);

    /*
     * Getting the list of deployments from the rpm-ostree DBus class and
     * converting each deployment to resource and detecting the currently running deployment.
     * It is corresponding to "rpm-ostree status".
     */
    void getDeployments();

    /*
     * Executing "rpm-ostree update --check" using QProcess to check if
     * there is a new deployment version avaliable.
     */
    void executeCheckUpdateProcess();

    /*
     * Getting the output resulting from executing the QProcess update check.
     */
    void getQProcessUpdateOutput(QByteArray readOutput);

    /*
     * Calling UpdateDeployment method from the rpm-ostree DBus class when
     * there is a new deployment version avaliable .
     */
    void updateCurrentDeployment();

    /*
     * Executing "ostree remote refs kinoite" using QProcess to get
     * a list of the avaliable remote refs list.
     */
    void executeRemoteRefsProcess();

    /*
     * Getting the output resulting from executing the QProcess remote refs list.
     */
    void getQProcessRefsOutput(QByteArray readOutput);

    /*
     * Setting the current remote refs list to the current running deployment resource.
     */
    void settingRemoteRefsDeploymentResource();

    /*
     * Calling Rebase method from the rpm-ostree DBus class when
     * there is a new kinoite refs.
     */
    void perfromSystemUpgrade(QString);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;

    // return true when OSTreeRPMBackend is ready to be loaded
    bool isValid() const override
    {
        return true;
    }

    Transaction *installApplication(AbstractResource *) override;
    Transaction *installApplication(AbstractResource *, const AddonList &) override;
    Transaction *removeApplication(AbstractResource *) override;
    bool isFetching() const override
    {
        return m_fetching;
    }
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;

public Q_SLOTS:
    void toggleFetching();

private:
    struct deploymentInformation {
        QString name;
        bool booted;
        QString baseVersion;
        QString baseChecksum;
        QString layeredPackages;
        QString localPackages;
        QString signature;
        QString origin;
    };

    QSharedPointer<OdrsReviewsBackend> m_reviews;
    StandardBackendUpdater *m_updater;
    QVector<RpmOstreeResource *> m_resources;
    QVector<deploymentInformation> deploymentsList;

    QString transactionUpdatePath;
    QStringList m_remoteRefsList;
    bool m_fetching;
    QString m_newVersion;

    /*
     * Checking if the required update is deployment update or system upgrade
     * by default isDeploymentUpdate is true
     */
    bool isDeploymentUpdate;

    /*
     * Extracting the new avaliable version of the deployment from the output
     * resulting from QProcess update deployment and setting the current running
     * deployment resource to AbstractResource::Upgradeable
     */
    void checkForUpdatesNeeded();
};

#endif