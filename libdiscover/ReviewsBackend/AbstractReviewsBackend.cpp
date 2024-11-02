/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AbstractReviewsBackend.h"
#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

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
        auto configGroup = KConfigGroup(config, u"Identity"_s);
        const QString configName = configGroup.readEntry("Name", QString());
        return configName.isEmpty() ? userName() : configName;
    }
}

ReviewsJob *AbstractReviewsBackend::submitReview(AbstractResource *resource,
                                                 const QString &summary,
                                                 const QString &reviewText,
                                                 const QString &rating,
                                                 const QString &userName)
{
    if (supportsNameChange() && !userName.isEmpty()) {
        auto config = KSharedConfig::openConfig();
        auto configGroup = KConfigGroup(config, u"Identity"_s);
        configGroup.writeEntry("Name", userName);
        configGroup.config()->sync();

        Q_EMIT preferredUserNameChanged();
    }
    return sendReview(resource, summary, reviewText, rating, userName);
}

QString AbstractReviewsBackend::errorMessage() const
{
    return QString();
}

#include "moc_AbstractReviewsBackend.cpp"
