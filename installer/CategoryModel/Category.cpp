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
{
    parseData(data);
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
        }

        if (tempElement.tagName() == QLatin1String("Icon")) {
            m_iconString = tempElement.text();
        }

        if (tempElement.tagName() == QLatin1String("Menu")) {
            Category *subCategory = new Category(this, node);
            m_subCategories << subCategory;
        }

        node = node.nextSibling();
    }
}

QString Category::name() const
{
    return m_name;
}

QString Category::icon() const
{
    return m_iconString;
}

QStringList Category::orSections() const
{
    return m_orSections;
}

QStringList Category::notSections() const
{
    return m_notSections;
}

bool Category::hasSubCategories() const
{
    return m_hasSubCategories;
}

QList<Category *> Category::subCategories() const
{
    return m_subCategories;
}
