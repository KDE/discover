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

#include "ReviewsModel.h"
#include <ReviewsBackend/ReviewsBackend.h>
#include <ReviewsBackend/Review.h>
#include <QDebug>

ReviewsModel::ReviewsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_app(0)
    , m_backend(0)
    , m_lastPage(0)
    , m_canFetchMore(true)
{
    QHash<int, QByteArray> roles = roleNames();
    roles.insert(ShouldShow, "shouldShow");
    roles.insert(Reviewer, "reviewer");
    roles.insert(CreationDate, "date");
    roles.insert(UsefulnessTotal, "usefulnessTotal");
    roles.insert(UsefulnessFavorable, "usefulnessFavorable");
    roles.insert(Rating, "rating");
    roles.insert(Summary, "summary");
    setRoleNames(roles);
}

QVariant ReviewsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();
    switch(role) {
        case Qt::DisplayRole:
            return m_reviews.at(index.row())->reviewText();
        case ShouldShow:
            return m_reviews.at(index.row())->shouldShow();
        case Reviewer:
            return m_reviews.at(index.row())->reviewer();
        case CreationDate:
            return m_reviews.at(index.row())->creationDate();
        case UsefulnessTotal:
            return m_reviews.at(index.row())->usefulnessTotal();
        case UsefulnessFavorable:
            return m_reviews.at(index.row())->usefulnessFavorable();
        case Rating:
            return m_reviews.at(index.row())->rating();
        case Summary:
            return m_reviews.at(index.row())->summary();
    }
    return QVariant();
}

int ReviewsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return m_reviews.count();
}

Application* ReviewsModel::application() const
{
    return m_app;
}

ReviewsBackend* ReviewsModel::backend() const
{
    return m_backend;
}

void ReviewsModel::setApplication(Application* app)
{
    if(m_app!=app) {
        reset();
        m_reviews.clear();

        m_app = app;
        restartFetching();
    }
}

void ReviewsModel::setBackend(ReviewsBackend* backend)
{
    if(m_backend) {
        disconnect(m_backend, SIGNAL(reviewsReady(Application*,QList<Review*>)),
            this, SLOT(addReviews(Application*,QList<Review*>)));
    }
    m_backend = backend;
    connect(m_backend, SIGNAL(reviewsReady(Application*,QList<Review*>)),
            this, SLOT(addReviews(Application*,QList<Review*>)));
    restartFetching();
}

void ReviewsModel::restartFetching()
{
    if(!m_app || !m_backend)
        return;

    m_canFetchMore=true;
    m_lastPage = 0;
    fetchMore();
}

void ReviewsModel::fetchMore(const QModelIndex& parent)
{
    if(!m_backend || !m_app || m_backend->isFetching() || parent.isValid() || !m_canFetchMore)
        return;

    m_lastPage++;
    m_backend->fetchReviews(m_app, m_lastPage);
    qDebug() << "fetching reviews... " << m_lastPage;
}

void ReviewsModel::addReviews(Application* app, const QList<Review*>& reviews)
{
    m_canFetchMore=!reviews.isEmpty();
    if(app!=m_app || reviews.isEmpty())
        return;
    qDebug() << "reviews arrived..." << m_lastPage << reviews.size();
    
    beginInsertRows(QModelIndex(), rowCount(), rowCount()+reviews.size()-1);
    m_reviews += reviews;
    endInsertRows();
}

bool ReviewsModel::canFetchMore(const QModelIndex&) const
{
    return m_canFetchMore;
}

void ReviewsModel::markUseful(int row, bool useful)
{
    Review* r = m_reviews[row];
    m_backend->submitUsefulness(r, useful);
}

void ReviewsModel::deleteReview(int row)
{
    Review* r = m_reviews[row];
    m_backend->deleteReview(r);
}

void ReviewsModel::flagReview(int row, const QString& reason, const QString& text)
{
    Review* r = m_reviews[row];
    m_backend->flagReview(r, reason, text);
}
