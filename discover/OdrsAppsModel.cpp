/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsAppsModel.h"
#include "appstream/AppStreamIntegration.h"
#include <ReviewsBackend/Rating.h>
#include <utils.h>

OdrsAppsModel::OdrsAppsModel()
{
    auto x = AppStreamIntegration::global()->reviews();
    connect(x.get(), &OdrsReviewsBackend::ratingsReady, this, &OdrsAppsModel::refresh);
    if (!x->top().isEmpty()) {
        refresh();
    }
}

void OdrsAppsModel::refresh()
{
    const auto top = AppStreamIntegration::global()->reviews()->top();
    setUris(kTransform<QVector<QUrl>>(top, [](auto r) {
        return QUrl("appstream://" + r->packageName());
    }));
}
