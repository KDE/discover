/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef REVIEWSMODEL_H
#define REVIEWSMODEL_H

#include <QModelIndex>
#include <QSharedPointer>
#include "discovercommon_export.h"

class Review;
typedef QSharedPointer<Review> ReviewPtr;

class AbstractResource;
class AbstractReviewsBackend;
class DISCOVERCOMMON_EXPORT ReviewsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AbstractReviewsBackend* backend READ backend NOTIFY resourceChanged)
    Q_PROPERTY(AbstractResource* resource READ resource WRITE setResource NOTIFY resourceChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)
    public:
        enum Roles {
            ShouldShow=Qt::UserRole+1,
            Reviewer,
            CreationDate,
            UsefulnessTotal,
            UsefulnessFavorable,
            UsefulChoice,
            Rating,
            Summary,
            Depth,
            PackageVersion
        };
        enum UserChoice {
            None,
            Yes,
            No
        };
        Q_ENUM(UserChoice)

        explicit ReviewsModel(QObject* parent = nullptr);
        ~ReviewsModel() override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        AbstractReviewsBackend* backend() const;
        void setResource(AbstractResource* app);
        AbstractResource* resource() const;
        void fetchMore(const QModelIndex& parent=QModelIndex()) override;
        bool canFetchMore(const QModelIndex& /*parent*/) const override;
        QHash<int, QByteArray> roleNames() const override;

    public Q_SLOTS:
        void deleteReview(int row);
        void flagReview(int row, const QString& reason, const QString& text);
        void markUseful(int row, bool useful);

    private Q_SLOTS:
        void addReviews(AbstractResource* app, const QVector<ReviewPtr>& reviews, bool canFetchMore);
        void restartFetching();

    Q_SIGNALS:
        void rowsChanged();
        void resourceChanged();

    private:
        AbstractResource* m_app;
        AbstractReviewsBackend* m_backend;
        QVector<ReviewPtr> m_reviews;
        int m_lastPage;
        bool m_canFetchMore;
};

#endif // REVIEWSMODEL_H

