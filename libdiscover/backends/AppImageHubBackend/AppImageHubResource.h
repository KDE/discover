#pragma once

#include <resources/AbstractResource.h>

class AppImageHubResource : public AbstractResource
{
    Q_OBJECT
public:
    AppImageHubResource(const QJsonObject &data, AbstractResourcesBackend *parent);

    QList<PackageState> addonsInformation() override { return {}; }
    QString section() override;
    QString origin() const override { return QStringLiteral("appimagehub"); }
    QString longDescription() override;
    QString availableVersion() const override { return QStringLiteral("latest"); }
    QString installedVersion() const override;
    QJsonArray licenses() override { return {}; }
    quint64 size() override { return 0; }
    bool hasCategory(const QString &category) const override;
    State state() override;
    QVariant icon() const override;
    QString comment() override { return m_comment; }
    QString name() const override { return m_name; }
    QString packageName() const override { return m_id; }
    QStringList mimetypes() const override { return {QStringLiteral("application/vnd.appimage"), QStringLiteral("application/x-executable")}; }
    Type type() const override { return Application; }
    bool canExecute() const override;
    void invokeApplication() const override;
    void fetchChangelog() override { Q_EMIT changelogFetched({}); }
    void fetchScreenshots() override { Q_EMIT screenshotsFetched(m_screenshots); }
    QUrl homepage() override { return m_homepage; }
    QString author() const override { return m_author; }
    QDate releaseDate() const override { return {}; }
    QString sourceIcon() const override { return QStringLiteral("application-x-executable"); }
    QUrl url() const override;

    QUrl downloadUrl() const { return m_downloadUrl; }
    bool hasDownloadUrl() const { return m_downloadUrl.isValid(); }
    QString installedPath() const;
    QString desktopFilePath() const;
    QString iconInstallPath() const;

private:
    QString localFileName() const;

    const QString m_id;
    const QString m_name;
    const QString m_comment;
    const QString m_longDescription;
    const QString m_author;
    const QStringList m_categories;
    const QUrl m_homepage;
    const QUrl m_downloadUrl;
    const QUrl m_iconUrl;
    const Screenshots m_screenshots;
};