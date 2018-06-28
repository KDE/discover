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
#include <QVariantList>
#include <QSet>

extern "C" {
#include <fwupd.h>
}
#include <fcntl.h>
#include <gio/gio.h>
#include <gio-unix-2.0/gio/gunixfdlist.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>


#include <libsoup/soup-request-http.h>
#include <libsoup/soup-requester.h>
#include <libsoup/soup.h>

#define FWUPD_DEBUG // UnComment This to see all debug messages

class QAction;
class FwupdReviewsBackend;
class FwupdUpdater;
class FwupdResource;
class FwupdBackend : public AbstractResourcesBackend
{
Q_OBJECT
Q_PROPERTY(int startElements MEMBER m_startElements)
public:
    explicit FwupdBackend(QObject* parent = NULL);
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
    GPtrArray *toDownload;
    GPtrArray *toIgnore;
    
    bool FwupdDownloadFile(const gchar *uri,const gchar *filename);
    GBytes* FwupdDownloadData(const gchar *uri); 
    bool FwupdRefreshRemotes(guint cache_age);
    bool FwupdRefreshRemote(FwupdRemote *remote,guint cache_age);
    gchar* FwupdCacheFile(const gchar *kind,const gchar *resource);
    bool FwupdDownloadAllScheduled(guint cache_age);
    bool FwupdAddToScheduleForDownload(const gchar * uri);
    FwupdResource * FwupdCreateDevice(FwupdDevice *device);
    FwupdResource * FwupdCreateRelease(FwupdDevice *device);
    bool FwupdAddToSchedule(FwupdDevice *device);
    gchar* FwupdGetChecksum(const gchar *filename,GChecksumType checksum_type);
    gchar* FwupdBuildDeviceID(FwupdDevice* device);
    void FwupdAddUpdates();
    void FwupdSetReleaseDetails(FwupdResource *res,FwupdRelease *rel);
    void FwupdSetDeviceDetails(FwupdResource *res,FwupdDevice *device);
    void FwupdHandleError(GError **perror);
    QSet<AbstractResource*> FwupdGetAllUpdates();
    QString FwupdGetAppName(QString ID);


public Q_SLOTS:
    void toggleFetching();

private:
    void populate(const QString& name);

    QHash<QString, FwupdResource*> m_resources;
    FwupdUpdater* m_updater;
    FwupdReviewsBackend* m_reviews;
    bool m_fetching;
    int m_startElements;
    QList<FwupdResource*> m_toUpdate;
    
    g_autofree gchar *userAgent = NULL;
    g_autoptr(SoupSession) soupSession = NULL;


};

#endif // FWUPDBACKEND_H
