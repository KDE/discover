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
#include <QtCore/QObject>

#include "libMuonCommon_export.h"

class QDomNode;

enum FilterType {
    InvalidFilter,
    CategoryFilter,
    PkgSectionFilter,
    PkgWildcardFilter,
    PkgNameFilter
};

class MUONCOMMON_EXPORT Category : public QObject
{
Q_OBJECT
public:
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(bool hasSubCategories READ hasSubCategories CONSTANT)
    Q_PROPERTY(bool shouldShowTechnical READ shouldShowTechnical CONSTANT)
    explicit Category(QObject* parent = 0);
    ~Category();

    QString name() const;
    QString icon() const;
    QList<QPair<FilterType, QString> > andFilters() const;
    QList<QPair<FilterType, QString> > orFilters() const;
    QList<QPair<FilterType, QString> > notFilters() const;
    bool hasSubCategories() const;
    bool shouldShowTechnical() const;
    QList<Category *> subCategories() const;

    static void addSubcategory(QList<Category*>& list, Category* cat);
    void parseData(const QString& path, const QDomNode& data, bool canHaveChildren);

private:
    QString m_name;
    QString m_iconString;
    QList<QPair<FilterType, QString> > m_andFilters;
    QList<QPair<FilterType, QString> > m_orFilters;
    QList<QPair<FilterType, QString> > m_notFilters;
    bool m_showTechnical;
    QList<Category *> m_subCategories;

    QList<QPair<FilterType, QString> > parseIncludes(const QDomNode &data);
};

#endif
