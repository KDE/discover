/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QVariant>

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT Rating
{
    Q_GADGET
    Q_PROPERTY(double sortableRating READ sortableRating CONSTANT)
    Q_PROPERTY(float rating READ rating CONSTANT)
    Q_PROPERTY(int ratingPoints READ ratingPoints CONSTANT)
    Q_PROPERTY(quint64 ratingCount READ ratingCount CONSTANT)
public:
    Rating()
    {
    }
    explicit Rating(const QString &packageName, quint64 ratingCount, int rating);
    explicit Rating(const QString &packageName, quint64 ratingCount, int data[6]);
    ~Rating();

    QString packageName() const;
    quint64 ratingCount() const;
    // 0.0 - 10.0 ranged rating
    float rating() const;
    int ratingPoints() const;
    // Returns a dampened rating calculated with the Wilson Score Interval algorithm
    double sortableRating() const;

private:
    QString m_packageName;
    quint64 m_ratingCount = 0;
    float m_rating = 0;
    int m_ratingPoints = 0;
    double m_sortableRating = 0;
};

Q_DECLARE_METATYPE(Rating)
