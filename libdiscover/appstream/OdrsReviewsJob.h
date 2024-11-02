/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "ReviewsBackend/AbstractReviewsBackend.h"
#include "ReviewsBackend/Review.h"
#include "resources/AbstractResource.h"
#include <QNetworkReply>

class OdrsReviewsJob : public ReviewsJob
{
    Q_OBJECT
public:
    ~OdrsReviewsJob();

    static OdrsReviewsJob *create(QNetworkReply *reply, AbstractResource *resource);

protected:
    OdrsReviewsJob(QNetworkReply *reply, AbstractResource *resource);
    void reviewsFetched();
    void parseReviews(const QJsonDocument &document);

    QNetworkReply *const m_reply;
    AbstractResource *const m_resource;
};

class OdrsSubmitReviewsJob : public OdrsReviewsJob
{
    Q_OBJECT
public:
    OdrsSubmitReviewsJob(QNetworkReply *reply, AbstractResource *resource);

    void reviewSubmitted();
};
