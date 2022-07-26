/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSReviews.h"
#include "KNSBackend.h"
#include "KNSResource.h"
#include <KLocalizedString>
#include <KNSCore/Engine>
#include <KPasswordDialog>
#include <QDebug>
#include <QDesktopServices>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <attica/content.h>
#include <attica/providermanager.h>
#include <resources/AbstractResource.h>

class SharedManager : public QObject
{
    Q_OBJECT
public:
    SharedManager()
    {
        atticaManager.loadDefaultProviders();
    }

public:
    Attica::ProviderManager atticaManager;
};

Q_GLOBAL_STATIC(SharedManager, s_shared)

KNSReviews::KNSReviews(KNSBackend *backend)
    : AbstractReviewsBackend(backend)
    , m_backend(backend)
{
}

Rating *KNSReviews::ratingForApplication(AbstractResource *app) const
{
    KNSResource *resource = qobject_cast<KNSResource *>(app);
    if (!resource) {
        qDebug() << app->packageName() << "<= couldn't find resource";
        return nullptr;
    }

    return resource->ratingInstance();
}

void KNSReviews::fetchReviews(AbstractResource *app, int page)
{
    Attica::ListJob<Attica::Comment> *job = provider().requestComments(Attica::Comment::ContentComment, app->packageName(), QStringLiteral("0"), page - 1, 10);
    if (!job) {
        Q_EMIT reviewsReady(app, {}, false);
        return;
    }
    job->setProperty("app", QVariant::fromValue<AbstractResource *>(app));
    connect(job, &Attica::BaseJob::finished, this, &KNSReviews::commentsReceived);
    job->start();
    m_fetching++;
}

static QVector<ReviewPtr> createReviewList(AbstractResource *app, const Attica::Comment::List comments, int depth = 0)
{
    QVector<ReviewPtr> reviews;
    for (const Attica::Comment &comment : comments) {
        // TODO: language lookup?
        ReviewPtr r(new Review(app->name(),
                               app->packageName(),
                               QStringLiteral("en"),
                               comment.subject(),
                               comment.text(),
                               comment.user(),
                               comment.date(),
                               true,
                               comment.id().toInt(),
                               comment.score() / 10,
                               0,
                               0,
                               QString()));
        r->addMetadata(QStringLiteral("NumberOfParents"), depth);
        reviews += r;
        if (comment.childCount() > 0) {
            reviews += createReviewList(app, comment.children(), depth + 1);
        }
    }
    return reviews;
}

void KNSReviews::commentsReceived(Attica::BaseJob *j)
{
    m_fetching--;
    Attica::ListJob<Attica::Comment> *job = static_cast<Attica::ListJob<Attica::Comment> *>(j);

    AbstractResource *app = job->property("app").value<AbstractResource *>();
    QVector<ReviewPtr> reviews = createReviewList(app, job->itemList());

    Q_EMIT reviewsReady(app, reviews, !reviews.isEmpty());
}

bool KNSReviews::isFetching() const
{
    return m_fetching > 0;
}

void KNSReviews::flagReview(Review * /*r*/, const QString & /*reason*/, const QString & /*text*/)
{
    qWarning() << "cannot flag reviews";
}

void KNSReviews::deleteReview(Review * /*r*/)
{
    qWarning() << "cannot delete comments";
}

void KNSReviews::submitReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating)
{
    provider().voteForContent(app->packageName(), rating.toUInt() * 20);
    if (!summary.isEmpty())
        provider().addNewComment(Attica::Comment::ContentComment, app->packageName(), QString(), QString(), summary, review_text);
}

void KNSReviews::submitUsefulness(Review *r, bool useful)
{
    provider().voteForComment(QString::number(r->id()), useful * 5);
}

void KNSReviews::logout()
{
    bool b = provider().saveCredentials(QString(), QString());
    if (!b)
        qWarning() << "couldn't log out";
}

void KNSReviews::registerAndLogin()
{
    QDesktopServices::openUrl(provider().baseUrl());
}

void KNSReviews::login()
{
    KPasswordDialog *dialog = new KPasswordDialog;
    dialog->setPrompt(i18n("Log in information for %1", provider().name()));
    connect(dialog, &KPasswordDialog::gotUsernameAndPassword, this, &KNSReviews::credentialsReceived);
}

void KNSReviews::credentialsReceived(const QString &user, const QString &password)
{
    bool b = provider().saveCredentials(user, password);
    if (!b)
        qWarning() << "couldn't save" << user << "credentials for" << provider().name();
}

bool KNSReviews::hasCredentials() const
{
    return provider().hasCredentials();
}

QString KNSReviews::userName() const
{
    QString user, password;
    provider().loadCredentials(user, password);
    return user;
}

void KNSReviews::setProviderUrl(const QUrl &url)
{
#if KNEWSTUFFCORE_VERSION < QT_VERSION_CHECK(5, 92, 0)
    m_providerUrl = url;
    if (!m_providerUrl.isEmpty() && !s_shared->atticaManager.providerFiles().contains(url)) {
        s_shared->atticaManager.addProviderFile(url);
    }
#endif
}

Attica::Provider KNSReviews::provider() const
{
#if KNEWSTUFFCORE_VERSION < QT_VERSION_CHECK(5, 92, 0)
    return s_shared->atticaManager.providerFor(m_providerUrl);
#else
    if (m_backend->engine()->atticaProviders().isEmpty()) {
        return {};
    }
    return *m_backend->engine()->atticaProviders().constFirst();
#endif
}

bool KNSReviews::isResourceSupported(AbstractResource *res) const
{
    return qobject_cast<KNSResource *>(res);
}

#include "KNSReviews.moc"
