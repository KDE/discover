/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include <KOSRelease>
#include <QObject>

class DISCOVERCOMMON_EXPORT AppStreamIntegration : public QObject
{
    Q_OBJECT
public:
    static AppStreamIntegration *global();

    KOSRelease *osRelease()
    {
        return &m_osrelease;
    }

private:
    KOSRelease m_osrelease;

    AppStreamIntegration()
    {
    }
};
