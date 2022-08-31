/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KConfigGroup>
#include <KSharedConfig>
#include "AbstractReviewsBackend.h"

AbstractReviewsBackend::AbstractReviewsBackend(QObject *parent)
    : QObject(parent)
{
}

bool AbstractReviewsBackend::isReviewable() const
{
    return true;
}

bool AbstractReviewsBackend::supportsNameChange() const
{
    return false;
}

QString AbstractReviewsBackend::preferredUserName() const
{
    if (!supportsNameChange()) {
        return userName();
    } else {
        auto config = KSharedConfig::openConfig();
        auto configGroup = KConfigGroup(config, "Identity");
        const QString configName = configGroup.readEntry("Name", QString());
        return configName.isEmpty() ? userName() : configName;
    }
}

void AbstractReviewsBackend::submitReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating, const QString &userName)
{
    if (supportsNameChange()) {
        auto config = KSharedConfig::openConfig();
        auto configGroup = KConfigGroup(config, "Identity");
        configGroup.writeEntry("Name", userName);
        configGroup.config()->sync();
    }
    sendReview(app, summary, review_text, rating, userName);
}

QString AbstractReviewsBackend::errorMessage() const
{
    return QString();
}
