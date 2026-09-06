#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QPointer>
#include <Transaction/Transaction.h>

class AppImageHubResource;
class AppImageHubTransaction : public Transaction
{
    Q_OBJECT
public:
    AppImageHubTransaction(AppImageHubResource *resource, Role role);
    void cancel() override;

private Q_SLOTS:
    void startDownload(const QUrl &url);
    void replyFinished();
    void readyRead();
    void downloadProgress(qint64 received, qint64 total);

private:
    void fail(const QString &message);
    void finishInstall();
    void writeDesktopFile();

    QPointer<AppImageHubResource> m_resource;
    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
    QFile m_file;
    bool m_cancelled = false;
};