/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KNSBACKENDTEST_H
#define KNSBACKENDTEST_H

#include <QObject>
#include <QPointer>
#include "ReviewsBackend/ReviewsModel.h"

class AbstractResourcesBackend;
class AbstractResource;
class ResultsStream;

class KNSBackendTest : public QObject
{
    Q_OBJECT
    public:
        explicit KNSBackendTest(QObject* parent = nullptr);

    private Q_SLOTS:
        void testRetrieval();
        void testReviews();
        void testResourceByUrl();
        void testResourceByUrlResourcesModel();

    public Q_SLOTS:
        void reviewsArrived(AbstractResource *r, const QVector<ReviewPtr>& revs);

    private:
        QVector<AbstractResource*> getResources(ResultsStream* stream, bool canBeEmpty = false);
        QVector<AbstractResource*> getAllResources(AbstractResourcesBackend* backend);
        QPointer<AbstractResourcesBackend> m_backend;
        QPointer<AbstractResource> m_r;
        QVector<ReviewPtr> m_revs;
};

#endif // KNSBACKENDTEST_H
