/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#ifndef FWUPDBACKEND_H
#define FWUPDBACKEND_H

#include <resources/AbstractResourcesBackend.h>

#include <QString>
#include <QDir>
#include <QDebug>
#include <QThread>
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
#include <QEventLoop>


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
Q_ENUMS(Modes)
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
    AbstractResource * resourceForFile(const QUrl & ) override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;
    FwupdClient *client;

    void refreshRemotes();
    FwupdResource * createDevice(FwupdDevice *device);
    FwupdResource * createRelease(FwupdDevice *device);
    FwupdResource * createApp(FwupdDevice *device);
    QString buildDeviceID(FwupdDevice* device);
    void addUpdates();
    void addResourceToList(FwupdResource *res);
    void addHistoricalUpdates();
    void setReleaseDetails(FwupdResource *res, FwupdRelease *release);
    void setDeviceDetails(FwupdResource *res, FwupdDevice *device);
    void handleError(GError **perror);
    QSet<AbstractResource*> getAllUpdates();

public Q_SLOTS:
    void refresh();

private:
    static QMap<GChecksumType,QCryptographicHash::Algorithm> gchecksumToQChryptographicHash();
    static QString cacheFile(const QString &kind, const QString &baseName);
    static void refreshRemote(FwupdBackend* backend, FwupdRemote *remote, uint cacheAge);
    static QByteArray getChecksum(const QString &filename, QCryptographicHash::Algorithm hashAlgorithm);
    static bool downloadFile(const QUrl &uri, const QString &filename);

    void populate();

    QHash<QString, FwupdResource*> m_resources;
    StandardBackendUpdater* m_updater;
    bool m_fetching = false;
    int m_startElements;
    QList<AbstractResource*> m_toUpdate;
};

#endif // FWUPDBACKEND_H
