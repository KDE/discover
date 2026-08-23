/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "imageversions.h"

#include "sysupdateerror.h"
#include "transferconfig.h"

#include <KAuth/Action>
#include <KLocalizedString>

#include <QCollator>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDateTime>
#include <QLocale>
#include <QTimeZone>

#include <algorithm>
#include <utility>

using namespace Qt::StringLiterals;

namespace
{
constexpr quint64 sysupdateOffline = 1;

const QString sysupdateService = u"org.freedesktop.sysupdate1"_s;
const QString hostTarget = u"/org/freedesktop/sysupdate1/target/host"_s;
const QString targetInterface = u"org.freedesktop.sysupdate1.Target"_s;

QString versionDate(const QString &version)
{
    // Image versions are UTC timestamps, with or without seconds.
    QDateTime timestamp;
    for (const QString &format : {u"yyyyMMddhhmmss"_s, u"yyyyMMddhhmm"_s}) {
        timestamp = QDateTime::fromString(version, format);
        if (timestamp.isValid()) {
            break;
        }
    }

    if (!timestamp.isValid()) {
        return {};
    }

    timestamp.setTimeZone(QTimeZone::UTC);
    return QLocale().toString(timestamp.toLocalTime(), QLocale::ShortFormat);
}

QString saveErrorMessage(int code, const QString &detail)
{
    switch (code) {
    case SysupdateError::InvalidVersion:
        return i18nc("@info %1 is a version string", "Could not change which system versions are kept: %1 is not a valid version.", detail);
    case SysupdateError::NoTransferDefinitions:
        return i18nc("@info", "Could not change which system versions are kept: this system is no longer updated by systemd-sysupdate.");
    case SysupdateError::WriteFailed:
        return i18nc("@info %1 is a file path", "Could not change which system versions are kept: could not write %1.", detail);
    default:
        break;
    }

    if (detail.isEmpty()) {
        return i18nc("@info", "Could not change which system versions are kept.");
    }
    return i18nc("@info", "Could not change which system versions are kept: %1", detail);
}
}

bool ImageVersions::isSupported()
{
    return TransferConfig::hasDefinitions();
}

ImageVersions::ImageVersions(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ImageVersions::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_versions.count();
}

QVariant ImageVersions::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const Version &version = m_versions.at(index.row());

    switch (role) {
    case VersionRole:
        return version.version;
    case DateRole:
        return version.date;
    case PinnedRole:
        return version.pinned;
    case EnforcedRole:
        return version.enforced;
    case RunningRole:
        return version.running;
    }

    return {};
}

QHash<int, QByteArray> ImageVersions::roleNames() const
{
    return {
        {VersionRole, "version"},
        {DateRole, "date"},
        {PinnedRole, "pinned"},
        {EnforcedRole, "enforced"},
        {RunningRole, "running"},
    };
}

bool ImageVersions::isBusy() const
{
    return m_pendingOperations > 0;
}

bool ImageVersions::isIncomplete() const
{
    return m_incomplete;
}

QString ImageVersions::errorMessage() const
{
    return m_error;
}

bool ImageVersions::isSaveNeeded() const
{
    return pinnedVersions() != m_applied;
}

bool ImageVersions::isDefaults() const
{
    return m_pinned.isEmpty();
}

void ImageVersions::load()
{
    const TransferConfig config = TransferConfig::scan();

    setError(QString());
    m_running = TransferConfig::runningVersion();
    m_pinned = config.ownVersions();
    m_applied = config.appliedVersions();
    m_enforced = config.foreignVersions();
    setIncomplete(m_pinned != m_applied);

    rebuild(m_pinned + m_enforced);
    Q_EMIT configurationChanged();

    queryVersions();
}

void ImageVersions::save()
{
    if (!isSaveNeeded() || m_saveJob) {
        return;
    }

    const QSet<QString> versions = pinnedVersions();

    KAuth::Action action(u"org.kde.discover.sysupdate.save"_s);
    action.setHelperId(u"org.kde.discover.sysupdate"_s);
    action.setArguments({{u"versions"_s, QStringList(versions.cbegin(), versions.cend())}});

    m_saveJob = action.execute();

    connect(m_saveJob, &KJob::result, this, [this, versions] {
        if (m_saveJob->error()) {
            setError(saveErrorMessage(m_saveJob->error(), m_saveJob->errorText()));
        } else {
            setError(QString());
            m_applied = versions;
            setIncomplete(false);
        }

        m_saveJob.clear();
        endOperation();
        Q_EMIT configurationChanged();
    });

    beginOperation();
    m_saveJob->start();
}

void ImageVersions::defaults()
{
    if (m_pinned.isEmpty()) {
        return;
    }

    m_pinned.clear();

    for (int row = 0; row < m_versions.count(); ++row) {
        if (std::exchange(m_versions[row].pinned, false)) {
            const QModelIndex changed = index(row, 0);
            Q_EMIT dataChanged(changed, changed, {PinnedRole});
        }
    }

    Q_EMIT configurationChanged();
}

void ImageVersions::setPinned(int row, bool pinned)
{
    if (row < 0 || row >= m_versions.count()) {
        return;
    }

    Version &version = m_versions[row];
    if (version.pinned == pinned || version.enforced) {
        return;
    }

    version.pinned = pinned;
    if (pinned) {
        m_pinned.insert(version.version);
    } else {
        m_pinned.remove(version.version);
    }

    const QModelIndex changed = index(row, 0);
    Q_EMIT dataChanged(changed, changed, {PinnedRole});
    Q_EMIT configurationChanged();
}

QSet<QString> ImageVersions::pinnedVersions() const
{
    return m_pinned;
}

void ImageVersions::queryVersions()
{
    QDBusMessage message = QDBusMessage::createMethodCall(sysupdateService, hostTarget, targetInterface, u"List"_s);
    message << sysupdateOffline;

    beginOperation();

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();
        endOperation();

        const QDBusPendingReply<QStringList> reply = *call;
        if (reply.isError()) {
            setError(i18nc("@info", "Could not list installed system versions, so this list may be incomplete: %1", reply.error().message()));
            return;
        }

        const QStringList installed = reply.value();
        rebuild(QSet<QString>(installed.cbegin(), installed.cend()) + m_pinned + m_enforced);
    });
}

void ImageVersions::rebuild(QSet<QString> installed)
{
    installed.remove(QString());
    if (!m_running.isEmpty()) {
        installed.insert(m_running);
    }

    QStringList sorted(installed.cbegin(), installed.cend());

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(sorted.begin(), sorted.end(), [&collator](const QString &left, const QString &right) {
        return collator.compare(left, right) > 0;
    });

    beginResetModel();

    m_versions.clear();
    m_versions.reserve(sorted.count());
    for (const QString &name : std::as_const(sorted)) {
        Version version;
        version.version = name;
        version.date = versionDate(name);
        version.pinned = m_pinned.contains(name);
        version.enforced = m_enforced.contains(name);
        version.running = name == m_running;
        m_versions.append(version);
    }

    endResetModel();
}

void ImageVersions::beginOperation()
{
    if (m_pendingOperations++ == 0) {
        Q_EMIT busyChanged();
    }
}

void ImageVersions::endOperation()
{
    Q_ASSERT(m_pendingOperations > 0);

    if (--m_pendingOperations == 0) {
        Q_EMIT busyChanged();
    }
}

void ImageVersions::setIncomplete(bool incomplete)
{
    if (m_incomplete != incomplete) {
        m_incomplete = incomplete;
        Q_EMIT incompleteChanged();
    }
}

void ImageVersions::setError(const QString &message)
{
    if (m_error != message) {
        m_error = message;
        Q_EMIT errorMessageChanged();
    }
}

#include "moc_imageversions.cpp"
