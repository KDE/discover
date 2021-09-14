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

#include <QDebug>
#include <QStandardItemModel>

const QStringList RpmOstreeResource::m_objects({QStringLiteral("qrc:/qml/RemoteRefsButton.qml")});

RpmOstreeResource::RpmOstreeResource(const QMap<QString, QVariant>& map, RpmOstreeBackend *parent)
    : AbstractResource(parent)
    , m_state(AbstractResource::None)
{
    // Use pretty name from os-release information
    auto osrelease = AppStreamIntegration::global()->osRelease();
    m_prettyname = osrelease->prettyName();

    // Get everything else from rpm-ostree
    m_name = map.value(QStringLiteral("osname")).toString();
    m_version = map.value(QStringLiteral("base-version")).toString();
    m_timestamp = QDateTime::fromSecsSinceEpoch(map.value(QStringLiteral("base-timestamp")).toULongLong()).date();

    // Split remote and branch from origin
    m_origin = map.value(QStringLiteral("origin")).toString();
    auto split_ref = m_origin.split(':');
    if (split_ref.length() != 2) {
        qWarning() << "rpm-ostree-backend: Unknown origin format, ignoring:" << m_origin;
        m_remote = QStringLiteral("unkonwn");
        m_branch = QStringLiteral("unkonwn");
    } else {
        m_remote = split_ref[0];
        m_branch = split_ref[1];
    }

    // Split branch into name / version / arch / variant
    auto split_branch = m_branch.split('/');
    if (split_branch.length() < 4) {
        qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << m_branch;
        m_branchName = QStringLiteral("unkonwn");
        m_branchVersion = QStringLiteral("unkonwn");
        m_branchArch = QStringLiteral("unkonwn");
        m_branchVariant = QStringLiteral("unkonwn");
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

    // Use ostree as tld to differentiate those resources and append the current branch:
    // Example: ostree.fedora-34-x86-64-kinoite
    // https://freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-id-generic
    m_appstreamid = m_branch;
    m_appstreamid = QStringLiteral("ostree.") + m_appstreamid.replace('/', '-').replace('_', '-');

    // TODO: Extract signature information
    // TODO: Extract the list of layered packages

    connect(this, &RpmOstreeResource::buttonPressed, parent, &RpmOstreeBackend::perfromSystemUpgrade);
}

void RpmOstreeResource::setRemoteRefsList(QStringList remoteRefs)
{
    if (!m_remoteRefsList.isEmpty())
        m_remoteRefsList.clear();
    m_remoteRefsList = remoteRefs;
}

QString RpmOstreeResource::getRecentRemoteRefs()
{
    if (!isRecentRefsAvaliable())
        return {};
    QString recentRefs = m_recentRefs;
    QStringList str = recentRefs.split(QStringLiteral("/"));
    QString refs = QStringLiteral("Fedora Kinoite ") + str[1];
    return refs;
}

bool RpmOstreeResource::isRecentRefsAvaliable()
{
    QString currentRefsVersion = m_origin;
    QStringList str = currentRefsVersion.split(QStringLiteral("/"));
    int currentVersion = str[1].toInt();

    for (const QString &refs : m_remoteRefsList) {
        if (refs == m_origin)
            continue;
        QString refssV = refs;
        QStringList refsNumber = refssV.split(QStringLiteral("/"));
        int refsNumberV = refsNumber[1].toInt();
        if (refsNumberV <= currentVersion)
            continue;
        m_recentRefs = refs;
    }

    if (m_recentRefs.isEmpty())
        return false;
    return true;
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
    return QUrl("https://spins.fedoraproject.org/en/kde/");
}

QUrl RpmOstreeResource::helpURL()
{
    return QUrl("https://docs.fedoraproject.org/en-US/fedora-kinoite/");
}

QUrl RpmOstreeResource::bugURL()
{
    return QUrl("https://pagure.io/fedora-kde/SIG/issues");
}

QJsonArray RpmOstreeResource::licenses()
{
    return {QJsonObject{{QStringLiteral("name"), i18n("GPL and other licenses")},
                        {QStringLiteral("url"), QStringLiteral("https://fedoraproject.org/wiki/Legal:Licenses")}}};
}

QString RpmOstreeResource::longDescription()
{
    return i18n("Remote: ") + m_origin;
}

QString RpmOstreeResource::name() const
{
    return m_prettyname;
}

QString RpmOstreeResource::origin() const
{
    return QStringLiteral("rpm-ostree");
}

QString RpmOstreeResource::packageName() const
{
    return {};
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
    return {};
}

QString RpmOstreeResource::comment()
{
    return i18n("The currently running version of Fedora Kinoite.");
}

int RpmOstreeResource::size()
{
    return 0;
}

QDate RpmOstreeResource::releaseDate() const
{
    return m_timestamp;
}

QString RpmOstreeResource::executeLabel() const
{
    return {};
}

void RpmOstreeResource::setState(AbstractResource::State state)
{
    m_state = state;
    Q_EMIT stateChanged();
}

void RpmOstreeResource::rebaseToNewVersion()
{
    Q_EMIT buttonPressed(m_recentRefs);
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

QString RpmOstreeResource::getRemote()
{
    return m_remote;
}

QString RpmOstreeResource::getBranchName()
{
    return m_branchName;
}

QString RpmOstreeResource::getBranchVersion()
{
    return m_branchVersion;
}

QString RpmOstreeResource::getBranchArch()
{
    return m_branchArch;
}

QString RpmOstreeResource::getBranchVariant()
{
    return m_branchVariant;
}
