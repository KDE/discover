/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "AbstractAppsModel.h"
#include <QPointer>
#include <qqmlintegration.h>

namespace KIO
{
class StoredTransferJob;
}

class FeaturedModel : public AbstractAppsModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    FeaturedModel();
    ~FeaturedModel() override
    {
    }

    void refresh() override;
};
