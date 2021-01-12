/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FWUPDBACKEND_H
#define FWUPDBACKEND_H

#include <resources/AbstractResourcesBackend.h>

#include <QString>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QMimeDatabase>
#include <QVariantList>
#include <QSet>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QMap>

extern "C" {
#include <fwupd.h>
}
#include <glib-2.0/glib-object.h>

class QAction;
class StandardBackendUpdater;
class FwupdResource;
class FwupdBackend : public AbstractResourcesBackend
{
Q_OBJECT
Q_PROPERTY(int startElements MEMBER m_startElements)
public:
    explicit FwupdBackend(QObject* parent = nullptr);
    ~FwupdBackend();

    int updatesCount() const override;
    AbstractBackendUpdater* backendUpdater() const override;
    AbstractReviewsBackend* reviewsBackend() const override;
    ResultsStream* search(const AbstractResourcesBackend::Filters & search) override;
    ResultsStream * findResourceByPackageName(const QUrl& search) ;
    QHash<QString, FwupdResource*> resources() const { return m_resources; }
    bool isValid() const override { return true; } // No external file dependencies that could cause runtime errors

    Transaction* installApplication(AbstractResource* app) override;
    Transaction* installApplication(AbstractResource* app, const AddonList& addons) override;
    Transaction* removeApplication(AbstractResource* app) override;
    bool isFetching() const override { return m_fetching; }
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;
    FwupdClient *client;
    void handleError(GError *perror);

    static QString cacheFile(const QString &kind, const QString &baseName);
    void setDevices(GPtrArray*);
    void setRemotes(GPtrArray*);

Q_SIGNALS:
    void initialized();

private:
    ResultsStream* resourceForFile(const QUrl & );
    void refreshRemotes();
    void addUpdates();
    void addResource(FwupdResource *res);
    QSet<AbstractResource*> getAllUpdates();

    static QMap<GChecksumType,QCryptographicHash::Algorithm> gchecksumToQChryptographicHash();
    static void refreshRemote(FwupdBackend* backend, FwupdRemote *remote, quint64 cacheAge, GCancellable *cancellable);
    static QByteArray getChecksum(const QString &filename, QCryptographicHash::Algorithm hashAlgorithm);

    FwupdResource * createRelease(FwupdDevice *device);
    FwupdResource * createApp(FwupdDevice *device);

    QHash<QString, FwupdResource*> m_resources;
    StandardBackendUpdater* m_updater;
    bool m_fetching = false;
    int m_startElements;
    QList<AbstractResource*> m_toUpdate;
    GCancellable *m_cancellable;
};

#endif // FWUPDBACKEND_H
