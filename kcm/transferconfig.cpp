/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "transferconfig.h"

#include <KOSRelease>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace
{
// Priority order
const QStringList configDirectories = {
    u"/etc/sysupdate.d"_s,
    u"/run/sysupdate.d"_s,
    u"/usr/local/lib/sysupdate.d"_s,
    u"/usr/lib/sysupdate.d"_s,
};

const QString writableDirectory = u"/etc/sysupdate.d"_s;
const QString dropInName = u"90-kcm-updates.conf"_s;

QString prefixed(const QString &root, const QString &path)
{
    return root.isEmpty() ? path : root + path;
}

bool isMasked(const QFileInfo &info)
{
    if (info.isSymLink() && info.symLinkTarget() == "/dev/null"_L1) {
        return true;
    }
    return info.size() == 0;
}

QMap<QString, QString> byFileName(const QString &root, const QStringList &directories, const QString &pattern)
{
    QMap<QString, QString> found;

    for (const QString &directory : directories) {
        const QDir dir(prefixed(root, directory));
        const QFileInfoList entries = dir.entryInfoList({pattern}, QDir::Files | QDir::System, QDir::Name);

        for (const QFileInfo &entry : entries) {
            if (!found.contains(entry.fileName())) {
                found.insert(entry.fileName(), isMasked(entry) ? QString() : entry.absoluteFilePath());
            }
        }
    }

    return found;
}

QStringList unmaskedPaths(const QMap<QString, QString> &found)
{
    QStringList paths;
    for (const QString &path : found) {
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    return paths;
}

QStringList definitionFiles(const QString &root)
{
    QMap<QString, QString> found = byFileName(root, configDirectories, u"*.transfer"_s);

    if (found.isEmpty()) {
        found = byFileName(root, configDirectories, u"*.conf"_s);
    }

    return unmaskedPaths(found);
}

QStringList configChain(const QString &root, const QString &definition)
{
    const QString dropInDirectory = u'/' + QFileInfo(definition).fileName() + ".d"_L1;

    QStringList directories;
    directories.reserve(configDirectories.size());
    for (const QString &directory : configDirectories) {
        directories.append(directory + dropInDirectory);
    }

    return QStringList{definition} + unmaskedPaths(byFileName(root, directories, u"*.conf"_s));
}

std::optional<QString> expandSpecifiers(const QString &value, const QString &runningVersion)
{
    QString expanded;
    expanded.reserve(value.size());

    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value.at(i) != u'%') {
            expanded.append(value.at(i));
            continue;
        }

        if (i + 1 == value.size()) {
            return std::nullopt;
        }

        const QChar specifier = value.at(++i);
        if (specifier == u'%') {
            expanded.append(u'%');
            continue;
        }

        // %A is the only specifier we know, and it only resolves when os-release actually
        // carries IMAGE_VERSION.
        if (specifier != u'A' || runningVersion.isEmpty()) {
            return std::nullopt;
        }

        expanded.append(runningVersion);
    }

    return expanded;
}

void replayProtectVersion(const QString &path, bool isOurs, const QString &runningVersion, QSet<QString> &own, QSet<QString> &foreign)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    bool inTransferSection = false;

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();

        if (line.startsWith(u'[')) {
            inTransferSection = line.compare("[Transfer]"_L1, Qt::CaseInsensitive) == 0;
            continue;
        }

        if (!inTransferSection || !line.startsWith("ProtectVersion="_L1, Qt::CaseInsensitive)) {
            continue;
        }

        const QString value = line.section(u'=', 1).trimmed();

        if (value.isEmpty()) {
            own.clear();
            foreign.clear();
            continue;
        }

        const std::optional<QString> version = expandSpecifiers(value, runningVersion);
        if (!version || version->isEmpty()) {
            continue;
        }

        if (isOurs) {
            own.insert(*version);
        } else {
            foreign.insert(*version);
        }
    }
}
}

TransferConfig TransferConfig::scan(const QString &root)
{
    TransferConfig config;

    const QString running = runningVersion(root);
    const QStringList definitions = definitionFiles(root);

    QSet<QString> expectedDropIns;

    for (const QString &definition : definitions) {
        Transfer transfer;
        transfer.definition = definition;
        transfer.dropIn = prefixed(root, u"%1/%2.d/%3"_s.arg(writableDirectory, QFileInfo(definition).fileName(), dropInName));
        expectedDropIns.insert(transfer.dropIn);

        const QStringList chain = configChain(root, definition);
        for (const QString &path : chain) {
            replayProtectVersion(path, path == transfer.dropIn, running, transfer.ownVersions, transfer.foreignVersions);
        }

        config.m_transfers.append(transfer);
    }

    const QDir writable(prefixed(root, writableDirectory));
    const QStringList entries = writable.entryList({u"*.d"_s}, QDir::Dirs);
    for (const QString &entry : entries) {
        const QString path = writable.filePath(entry + u'/' + dropInName);
        if (QFile::exists(path) && !expectedDropIns.contains(path)) {
            config.m_orphanedDropIns.append(path);
        }
    }

    return config;
}

bool TransferConfig::hasDefinitions(const QString &root)
{
    return !definitionFiles(root).isEmpty();
}

bool TransferConfig::isEmpty() const
{
    return m_transfers.isEmpty();
}

QList<TransferConfig::Transfer> TransferConfig::transfers() const
{
    return m_transfers;
}

QSet<QString> TransferConfig::ownVersions() const
{
    QSet<QString> versions;
    for (const Transfer &transfer : m_transfers) {
        versions.unite(transfer.ownVersions);
    }
    return versions;
}

QSet<QString> TransferConfig::appliedVersions() const
{
    if (m_transfers.isEmpty()) {
        return {};
    }

    QSet<QString> versions = m_transfers.constFirst().ownVersions;
    for (const Transfer &transfer : m_transfers) {
        versions.intersect(transfer.ownVersions);
    }
    return versions;
}

QSet<QString> TransferConfig::foreignVersions() const
{
    QSet<QString> versions;
    for (const Transfer &transfer : m_transfers) {
        versions.unite(transfer.foreignVersions);
    }
    return versions;
}

QStringList TransferConfig::orphanedDropIns() const
{
    return m_orphanedDropIns;
}

bool TransferConfig::isValidVersion(const QString &version)
{
    constexpr qsizetype nameMax = 255;

    if (version.isEmpty() || version.toUtf8().size() > nameMax) {
        return false;
    }

    if (version == "."_L1 || version == ".."_L1) {
        return false;
    }

    return std::all_of(version.cbegin(), version.cend(), [](QChar character) {
        const char16_t value = character.unicode();
        return (value >= u'0' && value <= u'9') //
            || (value >= u'a' && value <= u'z') //
            || (value >= u'A' && value <= u'Z') //
            || value == u'.' || value == u'-' || value == u'~' || value == u'^' || value == u'_' || value == u'+';
    });
}

QString TransferConfig::runningVersion(const QString &root)
{
    if (root.isEmpty()) {
        return KOSRelease().extraValue(u"IMAGE_VERSION"_s);
    }
    return KOSRelease(root + u"/etc/os-release"_s).extraValue(u"IMAGE_VERSION"_s);
}
