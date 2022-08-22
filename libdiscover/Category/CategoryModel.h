/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CATEGORYMODEL_H
#define CATEGORYMODEL_H

#include "Category.h"
#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "discovercommon_export.h"

class QTimer;

class DISCOVERCOMMON_EXPORT CategoryModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rootCategories READ rootCategoriesVL NOTIFY rootCategoriesChanged)
public:
    explicit CategoryModel(QObject *parent = nullptr);

    static CategoryModel *global();

    Q_SCRIPTABLE Category *findCategoryByName(const QString &name) const;
    void blacklistPlugin(const QString &name);
    QVector<Category *> rootCategories() const
    {
        return m_rootCategories;
    }
    QVariantList rootCategoriesVL() const;
    void populateCategories();

Q_SIGNALS:
    void rootCategoriesChanged();

private:
    QTimer *m_rootCategoriesChanged;
    QVector<Category *> m_rootCategories;
};

#endif // CATEGORYMODEL_H
