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
