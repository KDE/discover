/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include <QList>

class Category;
class AbstractResourcesBackend;
class DISCOVERCOMMON_EXPORT CategoriesReader
{
public:
    QList<Category *> loadCategoriesPath(const QString &path);
    QList<Category *> loadCategoriesFile(AbstractResourcesBackend *backend);
};
