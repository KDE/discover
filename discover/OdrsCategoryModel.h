/*
 *   SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "AbstractAppsModel.h"
#include "Category/Category.h"
#include <QPointer>
#include <QQmlParserStatus>

class OneTimeAction;

class OdrsCategoryModel : public AbstractAppsModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QString filteredCategoryName READ categoryName WRITE setCategoryName NOTIFY categoryChanged)
    Q_PROPERTY(uint pageSize MEMBER m_pageSize WRITE setPageSize NOTIFY pageSizeChanged)
    Q_INTERFACES(QQmlParserStatus)
public:
    OdrsCategoryModel();

    void refresh() override;
    void setCategory(const std::shared_ptr<Category> &category);
    void setCategoryName(const QString &name);

    QString categoryName() const
    {
        return m_category->name();
    }
    void setPageSize(uint pageSize)
    {
        if (m_pageSize == pageSize) {
            return;
        }
        m_pageSize = pageSize;
        Q_EMIT pageSizeChanged(pageSize);
        refresh();
    }

    void classBegin() override
    {
    }
    void componentComplete() override
    {
        m_initialized = true;
        refresh();
    }

Q_SIGNALS:
    void categoryChanged();
    void pageSizeChanged(uint pageSize);

private:
    std::shared_ptr<Category> m_category;
    QPointer<OneTimeAction> m_delayed;
    bool m_initialized = false;
    uint m_pageSize = 20;
};
