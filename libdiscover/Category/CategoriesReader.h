/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "Category.h"
#include "discovercommon_export.h"
#include <QVector>

class AbstractResourcesBackend;
class DISCOVERCOMMON_EXPORT CategoriesReader
{
public:
    QVector<Category *> loadCategoriesFile(AbstractResourcesBackend *backend);

private:
    friend class CategoriesTest;
    QVector<Category *> loadCategoriesPath(const QString &path, Category::Localization localization);
};
