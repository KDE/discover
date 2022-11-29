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
    m_origin = map.value(QStringLiteral("origin")).toString();
    if (m_origin.isEmpty()) {
        // If it's emmpty, then look for container origin format.
        // See https://github.com/ostreedev/ostree-rs-ext and
        // https://coreos.github.io/rpm-ostree/container/
        m_origin = map.value(QStringLiteral("container-image-reference")).toString();
        auto split_ref = m_origin.split(':');
        if ((split_ref.length() < 2) || (split_ref.length() > 3)) {
            qWarning() << "rpm-ostree-backend: Unknown container-image-reference format, ignoring:" << m_origin;
            m_remote = QStringLiteral("unknown");
            m_branch = QStringLiteral("unknown");
        } else {
            if (split_ref[0] != QStringLiteral("ostree-unverified-registry")) {
                qWarning() << "rpm-ostree-backend: Unknown container-image-reference format, ignoring:" << m_origin;
                m_remote = QStringLiteral("unknown");
                m_branch = QStringLiteral("unknown");
            } else {
                m_remote = split_ref[1];
                if (split_ref.length() == 3) {
                    m_branch = split_ref[2];
                } else {
                    m_branch = QStringLiteral("latest");
                }
            }
        }
    } else {
        // Found "classic" ostreee origin format. Get remote and branch from it
        auto split_ref = m_origin.split(':');
        if (split_ref.length() != 2) {
            qWarning() << "rpm-ostree-backend: Unknown origin format, ignoring:" << m_origin;
            m_remote = QStringLiteral("unknown");
            m_branch = QStringLiteral("unknown");
        } else {
            m_remote = split_ref[0];
            m_branch = split_ref[1];
        }
    }

    // Split branch into name / version / arch / variant
    auto split_branch = m_branch.split('/');
    if (split_branch.length() < 4) {
        qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << m_branch;
        m_branchName = QStringLiteral("unknown");
        m_branchVersion = QStringLiteral("unknown");
        m_branchArch = QStringLiteral("unknown");
        m_branchVariant = QStringLiteral("unknown");
    } else {
        m_branchName = split_branch[0];
        m_branchVersion = split_branch[1];
        m_branchArch = split_branch[2];
        auto variant = split_branch[3];
        for (int i = 4; i < split_branch.size(); ++i) {
            variant += "/" + split_branch[i];
        }
        m_branchVariant = variant;
    }

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

    // Store base-commit and current commit to be able to differentiate between deployments
    m_base_checksum = map.value(QStringLiteral("base-checksum")).toString();
    m_checksum = map.value(QStringLiteral("checksum")).toString();
    if (m_checksum.isEmpty()) {
        m_checksum = m_base_checksum;
    }

    // Use ostree as tld to differentiate those resources and append the current branch:
    // Example: ostree.fedora-34-x86-64-kinoite
    // https://freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-id-generic
    m_appstreamid = m_branch;
    m_appstreamid = QStringLiteral("ostree.") + m_appstreamid.replace('/', '-').replace('_', '-') + "." + m_checksum;
}

bool RpmOstreeResource::setNewMajorVersion(const QString &newMajorVersion)
{
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
    QByteArray rem = m_remote.toLocal8Bit();
    res = ostree_repo_remote_list_refs(repo, rem.data(), &refs, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not get the list of refs for ostree repo:" << path;
        return false;
    }

    // Iterate over the remote refs, keeping only the branches matching our
    // variant, to find if there is a indeed a branch available for the new
    // version.
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, refs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        auto ref = QString((char *)key);
        // Split branch into name / version / arch / variant
        auto split_branch = ref.split('/');
        if (split_branch.length() < 4) {
            qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << ref;
            continue;
        } else {
            auto refVariant = split_branch[3];
            for (int i = 4; i < split_branch.size(); ++i) {
                refVariant += "/" + split_branch[i];
            }
            if (split_branch[0] == m_branchName && split_branch[2] == m_branchArch && refVariant == m_branchVariant) {
                // Look for the branch matching the newMajorVersion.
                // This assumes that ostree branch names are lowercase.
                QString branchVersion = split_branch[1];
                if (branchVersion == newMajorVersion.toLower()) {
                    m_nextMajorVersion = newMajorVersion;
                    return true;
                }
            }
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
    // This assumes that ostree branch names are lowercase.
    QString ref;
    QTextStream(&ref) << m_branchName.toLower() << '/' << m_nextMajorVersion.toLower() << '/' << m_branchArch.toLower() << '/' << m_variant.toLower();
    return ref;
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
    return m_remote.at(0).toUpper() + m_remote.mid(1);
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
