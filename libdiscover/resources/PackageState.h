/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PACKAGESTATE_H
#define PACKAGESTATE_H

#include "discovercommon_export.h"
#include <QString>

/**
 * The @class PackageState will be used to expose resources related to an @class AbstractResource.
 *
 * @see ApplicationAddonsModel
 */
class DISCOVERCOMMON_EXPORT PackageState
{
public:
    PackageState(QString packageName, QString name, QString description, bool installed);
    PackageState(const QString &name, const QString &description, bool installed);
    PackageState(const PackageState &ps);
    PackageState &operator=(const PackageState &other);

    QString packageName() const;
    QString name() const;
    QString description() const;
    bool isInstalled() const;
    void setInstalled(bool installed);

private:
    const QString m_packageName;
    const QString m_name;
    const QString m_description;
    bool m_installed;
};

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const PackageState &state);

#endif // PACKAGESTATE_H
