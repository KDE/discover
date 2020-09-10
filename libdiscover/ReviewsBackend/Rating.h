/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RATING_H
#define RATING_H

#include <QObject>
#include <QVariant>

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT Rating : public QObject
{
Q_OBJECT
Q_PROPERTY(double sortableRating READ sortableRating CONSTANT)
Q_PROPERTY(float rating READ rating CONSTANT)
Q_PROPERTY(int ratingPoints READ ratingPoints CONSTANT)
Q_PROPERTY(quint64 ratingCount READ ratingCount CONSTANT)
public:
    explicit Rating(const QString &packageName, quint64 ratingCount, int rating);
    explicit Rating(const QString &packageName, quint64 ratingCount, const QVariantMap &data);
    ~Rating() override;

    QString packageName() const;
    quint64 ratingCount() const;
    // 0.0 - 10.0 ranged rating
    float rating() const;
    int ratingPoints() const;
    // Returns a dampened rating calculated with the Wilson Score Interval algorithm
    double sortableRating() const;

private:
    const QString m_packageName;
    const quint64 m_ratingCount;
    const float m_rating;
    int m_ratingPoints;
    double m_sortableRating;
};

#endif
