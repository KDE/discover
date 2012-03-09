/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef REVIEWSMODEL_H
#define REVIEWSMODEL_H

#include <QModelIndex>

class Review;
class Application;
class ReviewsBackend;
class ReviewsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ReviewsBackend* backend READ backend WRITE setBackend)
    Q_PROPERTY(Application* application READ application WRITE setApplication)
    public:
        enum Roles {
            ShouldShow=Qt::UserRole+1,
            Reviewer,
            CreationDate,
            UsefulnessTotal,
            UsefulnessFavorable,
            Rating,
            Summary
        };
        explicit ReviewsModel(QObject* parent = 0);
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

        void setBackend(ReviewsBackend* backend);
        ReviewsBackend* backend() const;
        void setApplication(Application* app);
        Application* application() const;
        virtual void fetchMore(const QModelIndex& parent=QModelIndex());
        virtual bool canFetchMore(const QModelIndex&) const;

    public slots:
        void deleteReview(int row);
        void flagReview(int row, const QString& reason, const QString& text);
        void markUseful(int row, bool useful);

    private slots:
        void addReviews(Application* app, const QList<Review*>& reviews);

    private:
        void restartFetching();

        Application* m_app;
        ReviewsBackend* m_backend;
        QList<Review*> m_reviews;
        int m_lastPage;
        bool m_canFetchMore;
};

#endif // REVIEWSMODEL_H

