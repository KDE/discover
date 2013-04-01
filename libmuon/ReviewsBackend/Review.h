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

#ifndef REVIEW_H
#define REVIEW_H

#include <QtCore/QDateTime>
#include <QtCore/QVariant>

#include "ReviewsModel.h"
#include "libmuonprivate_export.h"

class AbstractResource;

class MUONPRIVATE_EXPORT Review
{
public:
    explicit Review(const QVariantMap &data);
    Review(const QString& name, const QString& pkgName, const QString& language, const QString& summary,
               const QString& reviewText, const QString& userName, const QDateTime& date, bool show, quint64 id,
               int rating, int usefulTotal, int usefulFavorable, const QString& packageVersion);
    ~Review();

    // Creation date determines greater than/less than
    bool operator<(const Review &rhs) const;
    bool operator>(const Review &rhs) const;

    QString applicationName() const;
    QString packageName() const;
    QString packageVersion() const;
    QString language() const;
    QString summary() const;
    QString reviewText() const;
    QString reviewer() const;
    QDateTime creationDate() const;
    bool shouldShow() const;
    quint64 id() const;
    int rating() const;
    int usefulnessTotal() const;
    int usefulnessFavorable() const;
    ReviewsModel::UserChoice usefulChoice() const;
    void setUsefulChoice(ReviewsModel::UserChoice useful);
    AbstractResource *package();

private:
    QString m_appName;
    QDateTime m_creationDate;
    bool m_shouldShow;
    quint64 m_id;
    QString m_language;
    QString m_packageName;
    int m_rating;
    QString m_reviewText;
    QString m_reviewer;
    int m_usefulnessTotal;
    int m_usefulnessFavorable;
    ReviewsModel::UserChoice m_usefulChoice;
    QString m_summary;
    QString m_packageVersion;

    AbstractResource *m_package;
};

#endif
