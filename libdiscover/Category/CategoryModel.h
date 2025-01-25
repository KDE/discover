/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "Category.h"
#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "discovercommon_export.h"

class QTimer;

class DISCOVERCOMMON_EXPORT CategoryModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<std::shared_ptr<Category>> rootCategories READ rootCategories NOTIFY rootCategoriesChanged)
public:
    explicit CategoryModel(QObject *parent = nullptr);

    static CategoryModel *global();

    Q_SCRIPTABLE std::shared_ptr<Category> findCategoryByName(const QString &name) const;
    void blacklistPlugin(const QString &name);
    const QList<std::shared_ptr<Category>> &rootCategories() const;
    void populateCategories();

    Q_SCRIPTABLE static QObject *get(const std::shared_ptr<Category> &ptr);

Q_SIGNALS:
    void rootCategoriesChanged();

private:
    QTimer *m_rootCategoriesChanged;
    QList<std::shared_ptr<Category>> m_rootCategories;
};
