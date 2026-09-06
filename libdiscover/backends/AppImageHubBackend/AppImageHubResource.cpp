#include "AppImageHubResource.h"

#include <KLocalizedString>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocument>
#include <resources/AbstractResourcesBackend.h>

static QString plainTextDescription(const QString &description)
{
    QTextDocument document;
    document.setHtml(description);
    return document.toPlainText().trimmed();
}

static QString shortDescription(const QString &description)
{
    // AppImageHub currently provides no separate summary field, so use the first paragraph of the description as the comment.
    const auto text = plainTextDescription(description);
    const auto firstParagraph = text.section(QRegularExpression(QStringLiteral("\\n\\s*\\n")), 0, 0).simplified();
    constexpr qsizetype maximumLength = 240;
    return firstParagraph.size() > maximumLength ? firstParagraph.left(maximumLength - 3).trimmed() + QStringLiteral("...") : firstParagraph;
}

static QString richTextDescription(const QString &description)
{
    QTextDocument document;
    document.setHtml(description);
    return document.toHtml();
}

static QUrl feedUrl(const QString &path)
{
    return QUrl(path.startsWith(QStringLiteral("http://")) || path.startsWith(QStringLiteral("https://"))
                    ? path
                    : QStringLiteral("https://appimage.github.io/database/") + path);
}

AppImageHubResource::AppImageHubResource(const QJsonObject &data, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_id(data.value(QStringLiteral("name")).toString())
    , m_name(data.value(QStringLiteral("name")).toString())
    , m_comment(shortDescription(data.value(QStringLiteral("description")).toString()))
    , m_longDescription(richTextDescription(data.value(QStringLiteral("description")).toString()))
    , m_author([&data] {
        const auto authors = data.value(QStringLiteral("authors")).toArray();
        return authors.isEmpty() ? QString() : authors.at(0).toObject().value(QStringLiteral("name")).toString();
    }())
    , m_categories([&data] {
        QStringList categories;
        for (const auto &category : data.value(QStringLiteral("categories")).toArray())
            categories.append(category.toString());
        return categories;
    }())
    , m_homepage([&data] {
        for (const auto &link : data.value(QStringLiteral("links")).toArray()) {
            const auto object = link.toObject();
            if (object.value(QStringLiteral("type")).toString().compare(QStringLiteral("GitHub"), Qt::CaseInsensitive) == 0) {
                const auto url = object.value(QStringLiteral("url")).toString();
                return QUrl(url.startsWith(QStringLiteral("http")) ? url : QStringLiteral("https://github.com/") + url);
            }
        }
        return QUrl();
    }())
    , m_downloadUrl([&data] {
        for (const auto &link : data.value(QStringLiteral("links")).toArray()) {
            const auto object = link.toObject();
            const auto url = object.value(QStringLiteral("url")).toString();
            if (url.endsWith(QStringLiteral(".appimage"), Qt::CaseInsensitive))
                return QUrl(url);
            if (object.value(QStringLiteral("type")).toString().compare(QStringLiteral("GitHub"), Qt::CaseInsensitive) == 0) {
                const auto repository = url.startsWith(QStringLiteral("http")) ? QUrl(url).path().mid(1) : url;
                if (repository.count(QLatin1Char('/')) == 1)
                    return QUrl(QStringLiteral("https://api.github.com/repos/") + repository + QStringLiteral("/releases/latest"));
            }
        }
        const auto name = data.value(QStringLiteral("name")).toString();
        return name.isEmpty() ? QUrl() : QUrl(QStringLiteral("https://appimage.github.io/data/") + QString::fromUtf8(QUrl::toPercentEncoding(name)));
    }())
    , m_iconUrl([&data] {
        const auto icons = data.value(QStringLiteral("icons")).toArray();
        return icons.isEmpty() ? QUrl() : feedUrl(icons.first().toString());
    }())
    , m_screenshots([&data] {
        Screenshots screenshots;
        for (const auto &screenshot : data.value(QStringLiteral("screenshots")).toArray())
            screenshots.append(feedUrl(screenshot.toString()));
        return screenshots;
    }())
{
}

QString AppImageHubResource::localFileName() const
{
    auto result = m_name;
    result.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    return result + QStringLiteral(".AppImage");
}

QString AppImageHubResource::installedPath() const
{
    // TODO: in future we would like to support "Applications" in XDG standard locations, but for now we just use the hardcoded directory.
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QStringLiteral("/Applications/") + localFileName();
}

QString AppImageHubResource::desktopFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QLatin1Char('/') + localFileName() + QStringLiteral(".desktop");
}

QString AppImageHubResource::iconInstallPath() const
{
    const auto pathParts = m_iconUrl.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.size() < 3 || pathParts.at(pathParts.size() - 3) != QStringLiteral("icons")) {
        return {};
    }

    const auto size = pathParts.at(pathParts.size() - 2);
    const auto fileName = pathParts.constLast();
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/icons/hicolor/") + size + QLatin1Char('/') + fileName;
}

AbstractResource::State AppImageHubResource::state()
{
    return QFile::exists(installedPath()) ? Installed : None;
}

bool AppImageHubResource::canExecute() const
{
    return QFile::exists(installedPath());
}

QString AppImageHubResource::installedVersion() const
{
    return QFile::exists(installedPath()) ? QStringLiteral("latest") : QString();
}

QString AppImageHubResource::section()
{
    return m_categories.value(0, QStringLiteral("Utility"));
}

QString AppImageHubResource::longDescription()
{
    return m_longDescription;
}

bool AppImageHubResource::hasCategory(const QString &category) const
{
    return m_categories.contains(category, Qt::CaseInsensitive);
}

QVariant AppImageHubResource::icon() const
{
    return m_iconUrl.isValid() ? QVariant::fromValue(m_iconUrl) : QVariant(QStringLiteral("application-x-executable"));
}

void AppImageHubResource::invokeApplication() const
{
    if (!QProcess::startDetached(installedPath(), {})) {
        Q_EMIT backend()->passiveMessage(i18n("Failed to launch %1.", name()));
    }
}

QUrl AppImageHubResource::url() const
{
    return QUrl(QStringLiteral("appimage://") + m_id);
}