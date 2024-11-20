/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QSortFilterProxyModel>
#include <QTimer>

/**
 * @class LimitedRowCountProxyModel
 *
 * This class can be used to create representations of only a chunk of a model.
 *
 * With this component it will be possible to create views that only show a page
 * of a model, instead of drawing all the elements in the model.
 */
class LimitedRowCountProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    /** Holds the number of elements that will fit in a page */
    Q_PROPERTY(int pageSize READ pageSize WRITE setPageSize NOTIFY pageSizeChanged REQUIRED)

public:
    explicit LimitedRowCountProxyModel(QObject *object = nullptr);
    ~LimitedRowCountProxyModel() override;
    Q_DISABLE_COPY_MOVE(LimitedRowCountProxyModel);

    [[nodiscard]] int pageSize() const;
    void setPageSize(int count);

Q_SIGNALS:
    void pageSizeChanged();

protected:
    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int m_pageSize = 1;
    bool m_connected = false;
    QTimer m_invalidateTimer;
};
