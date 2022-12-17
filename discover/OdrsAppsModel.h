/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "AbstractAppsModel.h"

class OdrsAppsModel : public AbstractAppsModel
{
    Q_OBJECT
public:
    OdrsAppsModel();

    void refresh() override;
};
