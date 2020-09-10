/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "Review.h"
#include <resources/ResourcesModel.h>

Review::Review(QString  name, QString  pkgName, QString  language, QString  summary,
               QString  reviewText, QString  userName, const QDateTime& date, bool show, quint64 id,
               int rating, int usefulTotal, int usefulFavorable, QString  packageVersion)
    : m_appName(std::move(name))
    , m_creationDate(date)
    , m_shouldShow(show)
    , m_id(id)
    , m_language(std::move(language))
    , m_packageName(std::move(pkgName))
    , m_rating(rating)
    , m_reviewText(std::move(reviewText))
    , m_reviewer(std::move(userName))
    , m_usefulnessTotal(usefulTotal)
    , m_usefulnessFavorable(usefulFavorable)
    , m_usefulChoice(ReviewsModel::None)
    , m_summary(std::move(summary))
    , m_packageVersion(std::move(packageVersion))
{}

Review::~Review() = default;

bool Review::operator<(const Review &other) const
{
    return m_creationDate < other.m_creationDate;
}

bool Review::operator>(const Review &other) const
{
    return m_creationDate > other.m_creationDate;
}

QString Review::applicationName() const
{
    return m_appName;
}

QString Review::packageName() const
{
    return m_packageName;
}

QString Review::packageVersion() const
{
    return m_packageVersion;
}

QString Review::language() const
{
    return m_language;
}

QString Review::summary() const
{
    return m_summary;
}

QString Review::reviewText() const
{
    return m_reviewText;
}

QString Review::reviewer() const
{
    return m_reviewer;
}

QDateTime Review::creationDate() const
{
    return m_creationDate;
}

bool Review::shouldShow() const
{
    return m_shouldShow;
}

quint64 Review::id() const
{
    return m_id;
}

int Review::rating() const
{
    return m_rating;
}

int Review::usefulnessTotal() const
{
    return m_usefulnessTotal;
}

int Review::usefulnessFavorable() const
{
    return m_usefulnessFavorable;
}

ReviewsModel::UserChoice Review::usefulChoice() const
{
    return m_usefulChoice;
}

void Review::setUsefulChoice(ReviewsModel::UserChoice useful)
{
    m_usefulChoice = useful;
}

void Review::addMetadata(const QString &key, const QVariant &value)
{
    m_metadata.insert(key, value);
}

QVariant Review::getMetadata(const QString &key)
{
    return m_metadata.value(key);
}
