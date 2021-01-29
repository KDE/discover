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

#ifndef AlpineAppstreamDataDownloader_H
#define AlpineAppstreamDataDownloader_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class KJob;
class KUiServerJobTracker;
namespace KIO
{
class Job;
class TransferJob;
}

/**
 * @class AppstreamDataDownloader
 *
 * @details The job of this class is to download appstream data
 * gzipped XMLs from some web server.
 *
 * Some distros (for example, Alpine Linux) do not provide
 * appstream data as installable package, instead they host it
 * on the internet (at https://appstream.alpinelinux.org) and
 * software center app has to download them on its own.
 *
 * Those files are not very large (from few kilobytes to couple
 * of megabytes) but we still need them.
 *
 * URLs to download archives from are stored in JSON files in
 * /usr/share/libdiscover/external-appstream-urls/ directory
 * (QStandardPaths::GenericDataLocation/libdiscover/external-appstream-urls
 * from C++/Qt code, ${DATA_INSTALL_DIR}/libdiscover/external-appstream-urls
 * from cmake).
 *
 * JSON file format:
 * ---------------------
 * {
 *      "urls": [
 *          "https://url1",
 *          "https://url2",
 *          ...
 *      ]
 * }
 * ---------------------
 * These JSON files, if needed, should be provided by the linux
 * distribution.
 *
 * This class can load any amount of those JSON files,
 * fetch URLs from them and download all files pointed by
 * those URLs to discover's cache directory:
 * "~/.cache/discover/external_appstream_data" (aka
 * QStandardPaths::CacheLocation). Use
 * getAppStreamCacheDir() to get this path.
 * If files are already present in cache and not outdated,
 * they are not downloaded again. Default cache expiration
 * time is 7 days and can be tweaked using
 * setCacheExpirePeriodSecs().
 */
class AppstreamDataDownloader: public QObject
{
    Q_OBJECT
public:
    explicit AppstreamDataDownloader(QObject *parent = nullptr);

    /**
     * Use return value of this function to add extra metadata
     * directories to AppStream loader, in case of AppStreamQt:
     * AppStream::Pool::addMetadataLocation().
     * This method creates a cache dir if it does not exist.
     * @return directory where downloaded files are stored.
     */
    static QString appStreamCacheDir();

    /**
     * Call this after receiving downloadFinished() signal to
     * test if there actually was something new downloaded.
     * @return true, if new files were actually downloaded, or
     *         false if files already present in cache are up to date.
     */
    bool cacheWasUpdated() const { return m_cacheWasUpdated; }

    /**
     * @return cache expire timeout in seconds
     */
    qint64 cacheExpirePeriodSecs() const
    {
        return m_cacheExpireSeconds;
    }

    /**
     * Set cache expiration timeout.
     * @param secs - new cache expiration timeout, in seconds.
     */
    void setCacheExpirePeriodSecs(qint64 secs);

public Q_SLOTS:
    /**
     * Start the asynchronous download job.
     * downloadFinished() signal will be emitted when everything is done.
     * start() may finish immediately if all cached files are
     * up to date and no downloads are needed.
     */
    void start();

Q_SIGNALS:
    /**
     * This signal is emitted when download job is finished.
     * To check if there were actual downloads performed, call
     * cacheWasUpdated().
     */
    void downloadFinished();

private:
    QString calcLocalFileSavePath(const QUrl &urlTodownload);
    void loadUrlsJson(const QString &path);

private Q_SLOTS:
    void onJobData(KIO::Job *job, const QByteArray &data);
    void onJobResult(KJob *job);

protected:
    qint64 m_cacheExpireSeconds = 7 * 24 * 3600;
    QStringList m_urls;
    QStringList m_urlsToDownload;
    QHash<QString, QString> m_urlPrefixes;
    bool m_cacheWasUpdated = false;

    QList<KIO::TransferJob *> m_jobs;
    KUiServerJobTracker *m_jobTracker;
};

#endif
