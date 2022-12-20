/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeResource.h"
#include "RpmOstreeBackend.h"

#include <appstream/AppStreamIntegration.h>

#include <KLocalizedString>
#include <KOSRelease>

#include <ostree-repo.h>
#include <ostree.h>

RpmOstreeResource::RpmOstreeResource(const QVariantMap &map, RpmOstreeBackend *parent)
    : AbstractResource(parent)
    // All available deployments are by definition already installed
    , m_state(AbstractResource::Installed)
{
#ifdef QT_DEBUG
    qDebug() << "rpm-ostree-backend: Creating deployments from:";
    QMapIterator<QString, QVariant> iter(map);
    while (iter.hasNext()) {
        iter.next();
        qDebug() << "rpm-ostree-backend: " << iter.key() << ": " << iter.value();
    }
    qDebug() << "";
#endif

    // Get as much as possible from rpm-ostree
    m_osname = map.value(QStringLiteral("osname")).toString();

    // Look for the base-checksum first. This is the case where we have changes layered
    m_checksum = map.value(QStringLiteral("base-checksum")).toString();
    if (m_checksum.isEmpty()) {
        // If empty, look for the regular checksum (no layered changes)
        m_checksum = map.value(QStringLiteral("checksum")).toString();
    }

    // Look for the base-version first. This is the case where we have changes layered
    m_version = map.value(QStringLiteral("base-version")).toString();
    if (m_version.isEmpty()) {
        // If empty, look for the regular version (no layered changes)
        m_version = map.value(QStringLiteral("version")).toString();
    }

    // Look for the base-timestamp first. This is the case where we have changes layered
    auto timestamp = map.value(QStringLiteral("base-timestamp")).toULongLong();
    if (timestamp == 0) {
        // If "empty", look for the regular timestamp (no layered changes)
        timestamp = map.value(QStringLiteral("timestamp")).toULongLong();
    }
    if (timestamp == 0) {
        // If it's still empty, set an "empty" date
        m_timestamp = QDate();
    } else {
        // Otherwise, convert the timestamp to a date
        m_timestamp = QDateTime::fromSecsSinceEpoch(timestamp).date();
    }

    m_pinned = map.value(QStringLiteral("pinned")).toBool();
    m_pending = map.value(QStringLiteral("staged")).toBool();
    m_booted = map.value(QStringLiteral("booted")).toBool();

    if (m_booted) {
        // We can directly read the pretty name & variant from os-release
        // information if this is the currently booted deployment.
        auto osrelease = AppStreamIntegration::global()->osRelease();
        m_name = osrelease->name();
        m_variant = osrelease->variant();
        // Also extract the version if we could not find it earlier
        if (m_version.isEmpty()) {
            m_version = osrelease->versionId();
        }
    }

    // Look for "classic" ostree origin format first
    QString origin = map.value(QStringLiteral("origin")).toString();
    if (!origin.isEmpty()) {
        m_ostreeFormat.reset(new OstreeFormat(OstreeFormat::Format::Classic, origin));
        if (!m_ostreeFormat->isValid()) {
            // This should never happen
            qWarning() << "rpm-ostree-backend: Invalid origin for classic ostree format:" << origin;
        }
    } else {
        // Then look for OCI container format
        origin = map.value(QStringLiteral("container-image-reference")).toString();
        if (!origin.isEmpty()) {
            m_ostreeFormat.reset(new OstreeFormat(OstreeFormat::Format::OCI, origin));
            if (!m_ostreeFormat->isValid()) {
                // This should never happen
                qWarning() << "rpm-ostree-backend: Invalid reference for OCI container ostree format:" << origin;
            }
        } else {
            // This should never happen
            m_ostreeFormat.reset(new OstreeFormat(OstreeFormat::Format::Unknown, {}));
            qWarning() << "rpm-ostree-backend: Could not find a valid remote for this deployment:" << m_checksum;
        }
    }

    // Use ostree as tld for all ostree deployments and differentiate between them with the
    // remote/repo, ref/tag and commit..
    // Example: ostree.fedora.fedora-34-x86-64-kinoite.abcd1234567890
    // Example: ostree.quay.io-fedora-ostree-desktops-kinoite.abcd1234567890
    // https://freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-id-generic
    if (m_ostreeFormat->isClassic()) {
        m_appstreamid = m_ostreeFormat->remote() + "." + m_ostreeFormat->ref();
    } else if (m_ostreeFormat->isOCI()) {
        m_appstreamid = m_ostreeFormat->repo() + "." + m_ostreeFormat->tag();
    } else {
        m_appstreamid = QStringLiteral("");
    }
    m_appstreamid = QStringLiteral("ostree.") + m_appstreamid.replace('/', '-').replace('_', '-') + '.' + m_checksum;
#ifdef QT_DEBUG
    qInfo() << "rpm-ostree-backend: Found deployment:" << m_appstreamid;
#endif

    // Replaced & added packages
    m_requested_base_local_replacements = map.value(QStringLiteral("requested-base-local-replacements")).toStringList();
    m_requested_base_removals = map.value(QStringLiteral("requested-base-removals")).toStringList();
    m_requested_local_packages = map.value(QStringLiteral("requested-local-packages")).toStringList();
    m_requested_modules = map.value(QStringLiteral("requested-modules")).toStringList();
    m_requested_packages = map.value(QStringLiteral("requested-packages")).toStringList();
    m_requested_base_local_replacements.sort();
    m_requested_base_removals.sort();
    m_requested_local_packages.sort();
    m_requested_modules.sort();
    m_requested_packages.sort();

    // TODO: Extract signature information
}

bool RpmOstreeResource::setNewMajorVersion(const QString &newMajorVersion)
{
    if (!m_ostreeFormat->isValid()) {
        // Only operate on valid origin format
        return false;
    }

    // This check mostly makes sense for the classic Ostree format. Skip most of it for the
    // OCI  format case: it will fail later if the container tag does not exist.
    if (m_ostreeFormat->isOCI()) {
        // If we are using the latest tag then it means that we are not following a specific
        // major release and thus we don't need to rebase: it will automatically happen once
        // the latest tag points to a version build with the new major release.
        if (m_ostreeFormat->tag() == QStringLiteral("latest")) {
            return false;
        }

        // Set the new major version
        m_nextMajorVersion = newMajorVersion;
        // Replace the current version in the container tag by the new major version to find
        // the new tag to rebase to. This assumes that container tag names are lowercase.
        QString currentVersion = AppStreamIntegration::global()->osRelease()->versionId();
        m_nextMajorVersionRef = m_ostreeFormat->tag().replace(currentVersion, newMajorVersion.toLower(), Qt::CaseInsensitive);
        return true;
    }

    // Assume we're using the classic format from now on
    if (!m_ostreeFormat->isClassic()) {
        // Only operate on valid origin format
        return false;
    }

    // Fetch the list of refs available on the remote for the deployment
    g_autoptr(GFile) path = g_file_new_for_path("/ostree/repo");
    g_autoptr(OstreeRepo) repo = ostree_repo_new(path);
    if (repo == NULL) {
        qWarning() << "rpm-ostree-backend: Could not find ostree repo:" << path;
        return false;
    }

    g_autoptr(GError) err = NULL;
    gboolean res = ostree_repo_open(repo, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not open ostree repo:" << path;
        return false;
    }

    g_autoptr(GHashTable) refs;
    QByteArray rem = m_ostreeFormat->remote().toLocal8Bit();
    res = ostree_repo_remote_list_refs(repo, rem.data(), &refs, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not get the list of refs for ostree repo:" << path;
        return false;
    }

    // Replace the current version in current branch by the new major version to find the
    // new branch. This assumes that ostree branch names are lowercase.
    QString currentVersion = AppStreamIntegration::global()->osRelease()->versionId();
    QString newVersionBranch = m_ostreeFormat->ref().replace(currentVersion, newMajorVersion.toLower(), Qt::CaseInsensitive);

    // Iterate over the remote refs to verify that the new verions has a branch available
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, refs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        auto ref = QString((char *)key);
        if (ref == newVersionBranch) {
            m_nextMajorVersion = newMajorVersion;
            m_nextMajorVersionRef = newVersionBranch;
            return true;
        }
    }

    // If we reach here, it means that we could not find a matching branch. This
    // is unexpected and we should inform the user.
    qWarning() << "rpm-ostree-backend: Could not find a remote ref for the new major version in ostree repo";
    return false;
}

QString RpmOstreeResource::availableVersion() const
{
    return m_newVersion;
}

QString RpmOstreeResource::version()
{
    return m_version;
}

void RpmOstreeResource::setNewVersion(const QString &newVersion)
{
    m_newVersion = newVersion;
}

QString RpmOstreeResource::getNewVersion() const
{
    return m_newVersion;
}

QString RpmOstreeResource::getNextMajorVersion() const
{
    return m_nextMajorVersion;
}

QString RpmOstreeResource::getNextMajorVersionRef() const
{
    return m_nextMajorVersionRef;
}

QString RpmOstreeResource::appstreamId() const
{
    return m_appstreamid;
}

bool RpmOstreeResource::canExecute() const
{
    return false;
}

QVariant RpmOstreeResource::icon() const
{
    return QStringLiteral("application-x-rpm");
}

QString RpmOstreeResource::installedVersion() const
{
    return m_version;
}

QUrl RpmOstreeResource::url() const
{
    return QUrl();
}

QUrl RpmOstreeResource::donationURL()
{
    return QUrl();
}

QUrl RpmOstreeResource::homepage()
{
    return QUrl(AppStreamIntegration::global()->osRelease()->homeUrl());
}

QUrl RpmOstreeResource::helpURL()
{
    return QUrl(AppStreamIntegration::global()->osRelease()->documentationUrl());
}

QUrl RpmOstreeResource::bugURL()
{
    return QUrl(AppStreamIntegration::global()->osRelease()->bugReportUrl());
}

QJsonArray RpmOstreeResource::licenses()
{
    if (m_osname == QStringLiteral("fedora")) {
        return {QJsonObject{{QStringLiteral("name"), i18n("GPL and other licenses")},
                            {QStringLiteral("url"), QStringLiteral("https://fedoraproject.org/wiki/Legal:Licenses")}}};
    }
    return {QJsonObject{{QStringLiteral("name"), i18n("Unknown")}}};
}

QString RpmOstreeResource::longDescription()
{
    QString desc;
    if (!m_requested_packages.isEmpty()) {
        QTextStream(&desc) << i18n("Additional packages: ") << "\n<ul>";
        for (const QString &package : qAsConst(m_requested_packages)) {
            QTextStream(&desc) << "<li>" << package << "</li>\n";
        }
        QTextStream(&desc) << "</ul>\n";
    }
    if (!m_requested_modules.isEmpty()) {
        QTextStream(&desc) << i18n("Additional modules: ") << "\n<ul>";
        for (const QString &package : qAsConst(m_requested_modules)) {
            QTextStream(&desc) << "<li>" << package << "</li>\n";
        }
        QTextStream(&desc) << "</ul>\n";
    }
    if (!m_requested_local_packages.isEmpty()) {
        QTextStream(&desc) << i18n("Local packages: ") << "\n<ul>";
        for (const QString &package : qAsConst(m_requested_local_packages)) {
            QTextStream(&desc) << "<li>" << package << "</li>\n";
        }
        QTextStream(&desc) << "</ul>\n";
    }
    if (!m_requested_base_local_replacements.isEmpty()) {
        QTextStream(&desc) << i18n("Replaced packages:") << "\n<ul>";
        for (const QString &package : qAsConst(m_requested_base_local_replacements)) {
            QTextStream(&desc) << "<li>" << package << "</li>\n";
        }
        QTextStream(&desc) << "</ul>\n";
    }
    if (!m_requested_base_removals.isEmpty()) {
        QTextStream(&desc) << i18n("Removed packages:") << "\n<ul>";
        for (const QString &package : qAsConst(m_requested_base_removals)) {
            QTextStream(&desc) << "<li>" << package << "</li>\n";
        }
        QTextStream(&desc) << "</ul>\n";
    }
    if (m_pinned) {
        desc += "<br/>This version is pinned and won't be automatically removed on updates.";
    }
    return desc;
}

QString RpmOstreeResource::name() const
{
    return QStringLiteral("%1 %2").arg(packageName(), m_version);
}

QString RpmOstreeResource::origin() const
{
    if (m_ostreeFormat->isClassic()) {
        if (m_ostreeFormat->remote() == QStringLiteral("fedora")) {
            return QStringLiteral("Fedora Project");
        } else {
            return m_ostreeFormat->remote();
        }
    } else if (m_ostreeFormat->isOCI()) {
        return m_ostreeFormat->repo();
    }
    return i18n("Unknown");
}

QString RpmOstreeResource::packageName() const
{
    if (m_osname == QStringLiteral("fedora")) {
        return QStringLiteral("Fedora Kinoite");
    }
    return m_osname;
}

QString RpmOstreeResource::section()
{
    return {};
}

AbstractResource::State RpmOstreeResource::state()
{
    return m_state;
}

QString RpmOstreeResource::author() const
{
    if (m_osname == QStringLiteral("fedora")) {
        return QStringLiteral("Fedora Project");
    }
    return i18n("Unknown");
}

QString RpmOstreeResource::comment()
{
    if (m_booted) {
        if (m_pinned) {
            return i18n("Currently booted version (pinned)");
        } else {
            return i18n("Currently booted version");
        }
    } else if (m_pending) {
        return i18n("Version that will be used after reboot");
    } else if (m_pinned) {
        return i18n("Fallback version (pinned)");
    }
    return i18n("Fallback version");
}

quint64 RpmOstreeResource::size()
{
    return 0;
}

QString RpmOstreeResource::sizeDescription()
{
    return QStringLiteral("Unknown");
}

QDate RpmOstreeResource::releaseDate() const
{
    return m_timestamp;
}

void RpmOstreeResource::setState(AbstractResource::State state)
{
    m_state = state;
    Q_EMIT stateChanged();
}

QString RpmOstreeResource::sourceIcon() const
{
    return QStringLiteral("application-x-rpm");
}

QStringList RpmOstreeResource::extends() const
{
    return {};
}
AbstractResource::Type RpmOstreeResource::type() const
{
    return Technical;
}

bool RpmOstreeResource::isRemovable() const
{
    // TODO: Add support for pinning, un-pinning and removing a specific
    // deployments. Until we have that, we consider all deployments as pinned by
    // default (and thus non-removable).
    // return !m_booted && !m_pinned;
    return false;
}

QList<PackageState> RpmOstreeResource::addonsInformation()
{
    return QList<PackageState>();
}

QStringList RpmOstreeResource::categories()
{
    return {};
}

bool RpmOstreeResource::isBooted()
{
    return m_booted;
}

bool RpmOstreeResource::isPending()
{
    return m_pending;
}

bool RpmOstreeResource::isClassic()
{
    return m_ostreeFormat->isValid() && m_ostreeFormat->isClassic();
}

bool RpmOstreeResource::isOCI()
{
    return m_ostreeFormat->isValid() && m_ostreeFormat->isOCI();
}

QString RpmOstreeResource::OCIUrl()
{
    if (m_ostreeFormat->isValid() && m_ostreeFormat->isOCI()) {
        return QLatin1String("docker://") + m_ostreeFormat->repo() + ':' + m_ostreeFormat->tag();
        ;
    }
    // Should never happen
    return {};
}
