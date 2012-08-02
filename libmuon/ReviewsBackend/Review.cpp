/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "Review.h"

Review::Review(const QVariantMap &data)
{
    m_appName = data.value("app_name").toString();
    m_packageName = data.value("package_name").toString();
    m_packageVersion = data.value("version").toString();
    m_language = data.value("language").toString();
    m_summary = data.value("summary").toString();
    m_reviewText = data.value("review_text").toString();

    QString reviewUsername = data.value("reviewer_username").toString();
    QString reviewDisplayName = data.value("reviewer_displayname").toString();
    if (!reviewDisplayName.isEmpty()) {
        m_reviewer = reviewDisplayName;
    } else {
        m_reviewer = reviewUsername;
    }

    QString creationDate = data.value("date_created").toString();
    m_creationDate = QDateTime::fromString(creationDate, "yyyy-MM-dd HH:mm:ss");

    m_shouldShow = !data.value("hide").toBool();
    m_id = data.value("id").toULongLong();
    m_rating = data.value("rating").toInt() * 2;
    m_usefulnessTotal = data.value("usefulness_total").toInt();
    m_usefulnessFavorable = data.value("usefulness_favorable").toInt();
}

Review::Review(const QString& name, const QString& pkgName, const QString& language, const QString& summary,
               const QString& reviewText, const QString& userName, const QDateTime& date, bool show, quint64 id,
               int rating, int usefulTotal, int usefulFavorable, const QString& packageVersion)
    : m_appName(name)
    , m_creationDate(date)
    , m_shouldShow(show)
    , m_id(id)
    , m_language(language)
    , m_packageName(pkgName)
    , m_rating(rating)
    , m_reviewText(reviewText)
    , m_reviewer(userName)
    , m_usefulnessTotal(usefulTotal)
    , m_usefulnessFavorable(usefulFavorable)
    , m_summary(summary)
    , m_packageVersion(packageVersion)
{}

Review::~Review()
{
}

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

QApt::Package *Review::package() const
{
    return m_package;
}

void Review::setPackage(QApt::Package *package)
{
    m_package = package;
}
