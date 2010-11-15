/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef CATEGORY_H
#define CATEGORY_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

class QDomNode;

enum FilterType {
    InvalidFilter = 0,
    CategoryFilter = 1,
    PkgSectionFilter = 2,
    PkgWildcardFilter = 3,
    PkgNameFilter = 4
};

class Category
{
public:
    explicit Category(const QDomNode &node);
    ~Category();

    QString name() const;
    QString icon() const;
    QList<QPair<FilterType, QString> > andFilters() const;
    QList<QPair<FilterType, QString> > orFilters() const;
    QList<QPair<FilterType, QString> > notFilters() const;
    bool hasSubCategories() const;
    QList<Category *> subCategories() const;

private:
    QString m_name;
    QString m_iconString;
    QList<QPair<FilterType, QString> > m_andFilters;
    QList<QPair<FilterType, QString> > m_orFilters;
    QList<QPair<FilterType, QString> > m_notFilters;
    bool m_hasSubCategories;
    QList<Category *> m_subCategories;

    void parseData(const QDomNode &data);
    QList<QPair<FilterType, QString> > parseIncludes(const QDomNode &data);
};

#endif
