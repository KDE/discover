/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

class TransferConfig
{
public:
    struct Transfer {
        QString definition;
        QString dropIn;
        QSet<QString> ownVersions;
        QSet<QString> foreignVersions;
    };

    static TransferConfig scan(const QString &root = {});
    static bool hasDefinitions(const QString &root = {});

    bool isEmpty() const;
    QList<Transfer> transfers() const;

    /// Union across transfers.
    QSet<QString> ownVersions() const;
    /// Intersection across transfers.
    QSet<QString> appliedVersions() const;

    QSet<QString> foreignVersions() const;
    QStringList orphanedDropIns() const;

    static bool isValidVersion(const QString &version);
    static QString runningVersion(const QString &root = {});

private:
    QList<Transfer> m_transfers;
    QStringList m_orphanedDropIns;
};
