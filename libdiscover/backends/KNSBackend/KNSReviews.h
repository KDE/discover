/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KNSREVIEWS_H
#define KNSREVIEWS_H

#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <attica/provider.h>
#include <knewstuffcore_version.h>

class KNSBackend;
class QUrl;
namespace Attica
{
class BaseJob;
}

class KNSReviews : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    explicit KNSReviews(KNSBackend *backend);

    void fetchReviews(AbstractResource *app, int page = 1) override;
    bool isFetching() const override;
    void flagReview(Review *r, const QString &reason, const QString &text) override;
    void deleteReview(Review *r) override;
    void submitUsefulness(Review *r, bool useful) override;
    void logout() override;
    void registerAndLogin() override;
    void login() override;
    Rating *ratingForApplication(AbstractResource *app) const override;
    bool hasCredentials() const override;

    bool isResourceSupported(AbstractResource *res) const override;

protected:
    void sendReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating, const QString &userName) override;
    QString userName() const override;

private Q_SLOTS:
    void commentsReceived(Attica::BaseJob *job);
    void credentialsReceived(const QString &user, const QString &password);

private:
    Attica::Provider provider() const;
    void acquireFetching(bool f);

    KNSBackend *const m_backend;
    int m_fetching = 0;
};

#endif // KNSREVIEWS_H
