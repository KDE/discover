/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDateTime>
#include <QVariant>

#include "ReviewsModel.h"
#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT Review
{
public:
    Review(QString name,
           QString pkgName,
           QString language,
           QString summary,
           QString reviewText,
           QString userName,
           const QDateTime &date,
           bool show,
           quint64 id,
           int rating,
           int usefulTotal,
           int usefulFavorable,
           QString packageVersion);
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
    void addMetadata(const QString &key, const QVariant &value);
    QVariant getMetadata(const QString &key);

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
    QVariantMap m_metadata;
};
