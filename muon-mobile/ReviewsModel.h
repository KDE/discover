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

