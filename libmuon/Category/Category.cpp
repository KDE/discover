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

#include <QtXml/QDomNode>

#include <KLocale>
#include <KStandardDirs>
#include <QFile>

Category::Category(const QDomNode &data, CategoryChildPolicy policy)
        : m_iconString("applications-other")
        , m_hasSubCategories(false)
        , m_showTechnical(false)
        , m_policy(policy)
{
    parseData(data);
}

Category::~Category()
{}

void Category::parseData(const QDomNode &data)
{
    QDomNode node = data.firstChild();
    while(!node.isNull())
    {
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("Name")) {
            if (m_policy == CanHaveChildren) {
                m_name = i18nc("Category", tempElement.text().toUtf8());
            } else {
                m_name = i18nc("@label The label used for viewing all members of this category", "All");
            }
        } else if (tempElement.tagName() == QLatin1String("Icon")) {
            if (!tempElement.text().isEmpty()) {
                m_iconString = tempElement.text();
            }
        } else if (tempElement.tagName() == QLatin1String("ShowTechnical")) {
            m_showTechnical = true;
        } else if (tempElement.tagName() == QLatin1String("Menu")) {
            if (m_policy == CanHaveChildren) {
                Category *subCategory = new Category(node);
                subCategory->setParent(this);
                m_subCategories << subCategory;
                m_hasSubCategories = true;
            }
        } else if (tempElement.tagName() == QLatin1String("Include")) {
            parseIncludes(tempElement);
        }

        node = node.nextSibling();
    }

    if (m_hasSubCategories) {
        Category *allSubCategory = new Category(data, NoChildren);
        allSubCategory->setParent(this);
        m_subCategories << allSubCategory;
    }
}

QList<QPair<FilterType, QString> > Category::parseIncludes(const QDomNode &data)
{
    QDomNode node = data.firstChild();
    QList<QPair<FilterType, QString> > filter;
    while(!node.isNull())
    {
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("And")) {
            // Parse children
            m_andFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("Or")) {
            m_orFilters.append(parseIncludes(node));
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

QList<QPair<FilterType, QString> > Category::andFilters() const
{
    return m_andFilters;
}

QList<QPair<FilterType, QString> > Category::orFilters() const
{
    return m_orFilters;
}

QList<QPair<FilterType, QString> > Category::notFilters() const
{
    return m_notFilters;
}

bool Category::hasSubCategories() const
{
    return m_hasSubCategories;
}

bool Category::shouldShowTechnical() const
{
    return m_showTechnical;
}

QList<Category *> Category::subCategories() const
{
    return m_subCategories;
}

bool categoryLessThan(Category *c1, const Category *c2)
{
    return (QString::localeAwareCompare(c1->name(), c2->name()) < 0);
}

QList< Category* > Category::populateCategories()
{
    QFile menuFile(KStandardDirs::locate("data", "muon-installer/categories.xml"));
    QList<Category *> ret;

    if (!menuFile.open(QIODevice::ReadOnly)) {
        // Broken install or broken FS
        return ret;
    }

    QDomDocument menuDocument;
    QString error;
    int line;
    menuDocument.setContent(&menuFile, &error, &line);

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while(!node.isNull())
    {
        ret << new Category(node);

        node = node.nextSibling();
    }

    qSort(ret.begin(), ret.end(), categoryLessThan);
    
    return ret;
}
