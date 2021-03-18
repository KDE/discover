/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CATEGORIESREADER_H
#define CATEGORIESREADER_H

#include "discovercommon_export.h"
#include <QVector>

class Category;
class AbstractResourcesBackend;
class DISCOVERCOMMON_EXPORT CategoriesReader
{
public:
    QVector<Category *> loadCategoriesPath(const QString &path);
    QVector<Category *> loadCategoriesFile(AbstractResourcesBackend *backend);
};

#endif // CATEGORIESREADER_H
