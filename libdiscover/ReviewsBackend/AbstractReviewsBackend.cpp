/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractReviewsBackend.h"

AbstractReviewsBackend::AbstractReviewsBackend(QObject* parent)
    : QObject(parent)
{}

bool AbstractReviewsBackend::isReviewable() const
{
    return true;
}

QString AbstractReviewsBackend::errorMessage() const
{
    return QString();
}
