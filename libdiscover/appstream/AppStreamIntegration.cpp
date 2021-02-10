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
    QSharedPointer<OdrsReviewsBackend> ret;
    if (m_reviews) {
        ret = m_reviews;
    } else {
        ret = QSharedPointer<OdrsReviewsBackend>(new OdrsReviewsBackend());
        m_reviews = ret;
    }
    return ret;
}
