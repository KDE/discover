/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamIntegration.h"

AppStreamIntegration * AppStreamIntegration::global()
{
    static AppStreamIntegration * var = nullptr;
    if (!var) {
        var = new AppStreamIntegration;
    }

    return var;
}

QSharedPointer<OdrsReviewsBackend> AppStreamIntegration::reviews()
{
    if (!m_reviews) {
        m_reviews = QSharedPointer<OdrsReviewsBackend>::create();
    }
    return m_reviews;
}
