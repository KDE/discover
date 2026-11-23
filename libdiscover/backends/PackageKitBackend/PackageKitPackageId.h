/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QStringView>

namespace TransactionView
{
inline QStringView afterSeparators(QStringView packageId, int count)
{
    for (int i = 0; i < count; ++i) {
        const auto separator = packageId.indexOf(u';');
        if (separator < 0) {
            return {};
        }
        packageId = packageId.sliced(separator + 1);
    }
    return packageId;
}

inline QStringView packageName(QStringView packageId)
{
    return packageId.left(packageId.indexOf(u';'));
}

inline QStringView packageVersion(QStringView packageId)
{
    return packageName(afterSeparators(packageId, 1));
}

inline QStringView packageArch(QStringView packageId)
{
    return packageName(afterSeparators(packageId, 2));
}

inline QStringView packageData(QStringView packageId)
{
    return afterSeparators(packageId, 3);
}
}
