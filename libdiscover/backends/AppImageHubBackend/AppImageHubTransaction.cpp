#include "AppImageHubTransaction.h"
#include "AppImageHubResource.h"

#include <KLocalizedString>
#include <QDir>
#include <QFileDevice>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <resources/AbstractResourcesBackend.h>

static bool isDataLookup(const QNetworkReply *reply)
{
    return reply && reply->url().path().startsWith(QStringLiteral("/data/"));
}

AppImageHubTransaction::AppImageHubTransaction(AppImageHubResource *resource, Role role)
    : Transaction(resource->backend(), resource, role)
    , m_resource(resource)
{
    setCancellable(true);
    setStatus(QueuedStatus);
    QTimer::singleShot(0, this, [this, role] {
        if (!m_resource) {
            fail(i18n("The AppImage resource is no longer available."));
            return;
        }
        if (role == RemoveRole) {
            setStatus(CommittingStatus);
            QFile::remove(m_resource->installedPath());
            QFile::remove(m_resource->desktopFilePath());
            Q_EMIT m_resource->stateChanged();
            setProgress(100);
            setStatus(DoneStatus);
            return;
        }
        startDownload(m_resource->downloadUrl());
    });
}

void AppImageHubTransaction::startDownload(const QUrl &url)
{
    if (!url.isValid()) {
        fail(i18n("No AppImage download is available for %1.", m_resource->name()));
        return;
    }
    setStatus(DownloadingStatus);
    m_reply = m_manager.get(QNetworkRequest(url));
    connect(m_reply, &QNetworkReply::readyRead, this, &AppImageHubTransaction::readyRead);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &AppImageHubTransaction::downloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &AppImageHubTransaction::replyFinished);
}

void AppImageHubTransaction::readyRead()
{
    if (isDataLookup(m_reply))
        return;
    if (m_reply->header(QNetworkRequest::ContentTypeHeader).toString().contains(QStringLiteral("json")))
        return;
    if (!m_file.isOpen()) {
        QDir().mkpath(QFileInfo(m_resource->installedPath()).absolutePath());
        m_file.setFileName(m_resource->installedPath() + QStringLiteral(".part"));
        if (!m_file.open(QIODevice::WriteOnly)) {
            fail(i18n("Could not open %1 for writing.", m_resource->installedPath()));
            return;
        }
    }
    m_file.write(m_reply->readAll());
}

void AppImageHubTransaction::downloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
        setProgress(static_cast<int>(received * 100 / total));
}

void AppImageHubTransaction::replyFinished()
{
    if (m_cancelled || !m_reply)
        return;
    if (m_reply->error() != QNetworkReply::NoError) {
        fail(m_reply->errorString());
        return;
    }

    const auto data = m_reply->readAll();
    if (isDataLookup(m_reply)) {
        const auto downloadUrl = QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("\\r?\\n")), Qt::SkipEmptyParts).value(0).trimmed();
        if (downloadUrl.isEmpty()) {
            fail(i18n("No AppImage download URL was found in the application data."));
            return;
        }
        startDownload(QUrl(downloadUrl));
        return;
    }
    if (!m_file.isOpen() && m_reply->header(QNetworkRequest::ContentTypeHeader).toString().contains(QStringLiteral("json"))) {
        const auto assets = QJsonDocument::fromJson(data).object().value(QStringLiteral("assets")).toArray();
        for (const auto &asset : assets) {
            const auto object = asset.toObject();
            if (object.value(QStringLiteral("name")).toString().endsWith(QStringLiteral(".appimage"), Qt::CaseInsensitive)) {
                startDownload(QUrl(object.value(QStringLiteral("browser_download_url")).toString()));
                return;
            }
        }
        fail(i18n("No AppImage asset was found in the latest release."));
        return;
    }

    if (m_file.isOpen()) {
        m_file.close();
        if (!QFile::rename(m_file.fileName(), m_resource->installedPath())) {
            fail(i18n("Could not install %1.", m_resource->installedPath()));
            return;
        }
    }
    finishInstall();
}

void AppImageHubTransaction::finishInstall()
{
    setStatus(CommittingStatus);
    const auto iconPath = m_resource->iconInstallPath();
    if (!iconPath.isEmpty() && m_resource->icon().canConvert<QUrl>()) {
        const auto iconFile = iconPath;
        QDir().mkpath(QFileInfo(iconFile).absolutePath());
        auto *iconReply = m_manager.get(QNetworkRequest(m_resource->icon().toUrl()));
        connect(iconReply, &QNetworkReply::finished, this, [this, iconReply, iconFile] {
            if (iconReply->error() == QNetworkReply::NoError) {
                QFile file(iconFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    file.write(iconReply->readAll());
                }
            }
            iconReply->deleteLater();
            writeDesktopFile();
        });
        return;
    }
    writeDesktopFile();
}

void AppImageHubTransaction::writeDesktopFile()
{
    QFile::setPermissions(m_resource->installedPath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup
                              | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
    QDir().mkpath(QFileInfo(m_resource->desktopFilePath()).absolutePath());
    QFile desktop(m_resource->desktopFilePath());
    if (!desktop.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(i18n("Could not create the application menu entry."));
        return;
    }
    QTextStream stream(&desktop);
        const auto installedIcon = m_resource->iconInstallPath();
    stream << "[Desktop Entry]\nType=Application\nName=" << m_resource->name() << "\nComment=" << m_resource->comment()
            << "\nExec=\"" << m_resource->installedPath() << "\"\nIcon=" << (installedIcon.isEmpty() ? QStringLiteral("application-x-executable") : installedIcon)
            << "\nCategories=Utility;\n";
    Q_EMIT m_resource->stateChanged();
    setProgress(100);
    setStatus(DoneStatus);
}

void AppImageHubTransaction::fail(const QString &message)
{
    if (m_file.isOpen()) {
        m_file.close();
        QFile::remove(m_file.fileName());
    }
    Q_EMIT passiveMessage(message);
    setStatus(m_cancelled ? CancelledStatus : DoneWithErrorStatus);
}

void AppImageHubTransaction::cancel()
{
    m_cancelled = true;
    if (m_reply)
        m_reply->abort();
    fail(i18n("AppImage installation was cancelled."));
}