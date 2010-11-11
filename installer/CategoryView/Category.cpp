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

#include "Category.h"

#include <KDebug>

Category::Category(QObject *parent, const QDomNode &data)
        : QObject(parent)
        , m_iconString("applications-other")
        , m_hasSubCategories(false)
{
    parseData(data);
    kDebug() << m_andOrFilters;
    kDebug() << m_notFilters;
    kDebug() << QString("\n\n");
}

Category::~Category()
{
}

void Category::parseData(const QDomNode &data)
{
    QDomNode node = data.firstChild();
    while( !node.isNull() )
    {
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("Name")) {
            if (tempElement.hasAttribute("xml:lang")) {
                // Skip translated nodes. We'll look up the l10n later
                node = node.nextSibling();
                continue;
            }
            m_name = tempElement.text();
        } else if (tempElement.tagName() == QLatin1String("Icon")) {
            m_iconString = tempElement.text();
        } else if (tempElement.tagName() == QLatin1String("Menu")) {
            Category *subCategory = new Category(this, node);
            m_subCategories << subCategory;
            m_hasSubCategories = true;
        } else if (tempElement.tagName() == QLatin1String("Include")) {
            parseIncludes(tempElement);
        }

        node = node.nextSibling();
    }
}

QList<QPair<FilterType, QString> > Category::parseIncludes(const QDomNode &data)
{
    QDomNode node = data.firstChild();
    QList<QPair<FilterType, QString> > filter;
    while(!node.isNull())
    {
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("And") || tempElement.tagName() == QLatin1String("Or")) {
            // Parse children
            m_andOrFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("Not")) {
            m_notFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("PkgSection")) {
            QPair<FilterType, QString> pkgSectionFilter;
            pkgSectionFilter.first = PkgSectionFilter;
            pkgSectionFilter.second = tempElement.text();
            filter.append(pkgSectionFilter);
        } else if (tempElement.tagName() == QLatin1String("Category")) {
            QPair<FilterType, QString> categoryFilter;
            categoryFilter.first = CategoryFilter;
            categoryFilter.second = tempElement.text();
            filter.append(categoryFilter);
        } else if (tempElement.tagName() == QLatin1String("PkgWildcard")) {
            QPair<FilterType, QString> wildcardFilter;
            wildcardFilter.first = PkgWildcardFilter;
            wildcardFilter.second = tempElement.text();
            filter.append(wildcardFilter);
        } else if (tempElement.tagName() == QLatin1String("PkgName")) {
            QPair<FilterType, QString> nameFilter;
            nameFilter.first = PkgNameFilter;
            nameFilter.second = tempElement.text();
            filter.append(nameFilter);
        }
        node = node.nextSibling();
    }

    return filter;
}

QString Category::name() const
{
    return m_name;
}

QString Category::icon() const
{
    return m_iconString;
}

QList<QPair<FilterType, QString> > Category::andOrFilters() const
{
    return m_andOrFilters;
}

QList<QPair<FilterType, QString> > Category::notFilters() const
{
    return m_notFilters;
}

bool Category::hasSubCategories() const
{
    return m_hasSubCategories;
}

QList<Category *> Category::subCategories() const
{
    return m_subCategories;
}
