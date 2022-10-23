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

const QStringList RpmOstreeResource::m_objects({QStringLiteral("qrc:/qml/RemoteRefsButton.qml")});

RpmOstreeResource::RpmOstreeResource(const QVariantMap &map, RpmOstreeBackend *parent)
    : AbstractResource(parent)
    , m_state(AbstractResource::None)
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

    // All available deployments are by definition already installed.
    m_state = AbstractResource::Installed;

    // Get as much as possible from rpm-ostree
    m_osname = map.value(QStringLiteral("osname")).toString();
    m_version = map.value(QStringLiteral("base-version")).toString();
    m_timestamp = QDateTime::fromSecsSinceEpoch(map.value(QStringLiteral("base-timestamp")).toULongLong()).date();

    // Consider all deployments as pinned (and thus non-removable) until we
    // support for un-pinning and removing selected deployments.
    // TODO: Support for pinning, un-pinning and removing deployments
    // m_pined = map.value(QStringLiteral("pinned")).toBool();
    m_pinned = true;
    m_pending = map.value(QStringLiteral("staged")).toBool();
    m_booted = map.value(QStringLiteral("booted")).toBool();

    if (m_booted) {
        // We can directly read the pretty name & variant from os-release
        // information if this is the currently booted deployment.
        auto osrelease = AppStreamIntegration::global()->osRelease();
        m_name = osrelease->name();
        m_variant = osrelease->variant();
    }

    // Split remote and branch from origin
    m_origin = map.value(QStringLiteral("origin")).toString();
    auto split_ref = m_origin.split(':');
    if (split_ref.length() != 2) {
        qWarning() << "rpm-ostree-backend: Unknown origin format, ignoring:" << m_origin;
        m_remote = QStringLiteral("unknown");
        m_branch = QStringLiteral("unknown");
    } else {
        m_remote = split_ref[0];
        m_branch = split_ref[1];
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

    connect(this, &RpmOstreeResource::buttonPressed, parent, &RpmOstreeBackend::rebaseToNewVersion);
}

void RpmOstreeResource::fetchRemoteRefs()
{
    g_autoptr(GFile) path = g_file_new_for_path("/ostree/repo");
    g_autoptr(OstreeRepo) repo = ostree_repo_new(path);
    if (repo == NULL) {
        qWarning() << "rpm-ostree-backend: Could not find ostree repo:" << path;
        return;
    }

    g_autoptr(GError) err = NULL;
    gboolean res = ostree_repo_open(repo, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not open ostree repo:" << path;
        return;
    }

    g_autoptr(GHashTable) refs;
    QByteArray rem = m_remote.toLocal8Bit();
    res = ostree_repo_remote_list_refs(repo, rem.data(), &refs, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not get the list of refs for ostree repo:" << path;
        return;
    }

    // Clear out existing refs
    m_remoteRefs.clear();

    int currentVersion = m_branchVersion.toInt();

    // Iterate over the remote refs to keep only the branches matching our
    // variant and to find if there is a newer version available.
    // TODO: Implement new release detection as currently this will offer to
    // rebase to a newer version as soon as it is branched in the Fedora
    // release process which is well before the official release.
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
                // Add to the list of available refs
                m_remoteRefs.push_back(ref);
                // Look for the branch with the next version
                // This will fail to parse "rawhide" and return 0 and will thus skip it
                int version = split_branch[1].toInt();
                if (version == currentVersion + 1) {
                    m_nextMajorVersion = split_branch[1];
                }
            }
        }
    }

    if (m_remoteRefs.size() == 0) {
        qWarning() << "rpm-ostree-backend: Could not find any corresponding remote ref in ostree repo:" << path;
    }
}

QString RpmOstreeResource::getNextMajorVersion()
{
    return m_nextMajorVersion;
}

bool RpmOstreeResource::isNextMajorVersionAvailable()
{
    return m_nextMajorVersion != "";
}

QString RpmOstreeResource::availableVersion() const
{
    return m_newVersion;
}

void RpmOstreeResource::setNewVersion(QString newVersion)
{
    m_newVersion = newVersion;
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
        return i18n("Currently booted version");
    }
    if (m_pending) {
        return i18n("Next version used after reboot");
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

void RpmOstreeResource::rebaseToNewVersion()
{
    QString ref;
    QTextStream(&ref) << m_branchName << '/' << m_nextMajorVersion << '/' << m_branchArch << '/' << m_variant;
    Q_EMIT buttonPressed(ref);
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
    return !m_booted && !m_pinned;
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
