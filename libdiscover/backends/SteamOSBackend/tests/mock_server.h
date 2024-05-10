/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#ifndef MOCK_SERVER_H
#define MOCK_SERVER_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

#include "dbushelpers.h"

#define TEST_VERSION QLatin1String("6")
#define TEST_BUILDID QLatin1String("20250326.0")
#define TEST_BRANCH QLatin1String("main")

class MockServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString CurrentVersion READ currentVersion)
    Q_PROPERTY(QString CurrentBuildID READ currentBuildID)
    Q_PROPERTY(QString Branch READ branch)
public:
    QString currentVersion() const;
    QString currentBuildID() const;
    QString branch() const;

    VariantMapMap CheckForUpdates(const QVariantMap &options, VariantMapMap &updates_available_later);
};

#endif
