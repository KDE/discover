/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsAppsModel.h"
#include <ReviewsBackend/Rating.h>
#include <appstream/OdrsReviewsBackend.h>
#include <utils.h>

using namespace Qt::StringLiterals;

OdrsAppsModel::OdrsAppsModel()
{
    auto x = OdrsReviewsBackend::global();
    connect(x.get(), &OdrsReviewsBackend::ratingsReady, this, &OdrsAppsModel::refresh);
    if (!x->top().isEmpty()) {
        refresh();
    }
}

void OdrsAppsModel::refresh()
{
    const auto top = OdrsReviewsBackend::global()->top();
    setUris(kTransform<QList<QUrl>>(top, [](auto r) {
        return QUrl("appstream://"_L1 + r->packageName());
    }));
}
