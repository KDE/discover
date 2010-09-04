/***************************************************************************
 *   Copyright (C) 2009 by Rafael FernÃ¡ndez LÃ³pez <ereslibre@kde.org>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#ifndef CATEGORYDRAWER_H
#define CATEGORYDRAWER_H

#include <KCategoryDrawer>

class QPainter;
class QModelIndex;
class QStyleOption;

class CategoryDrawer : public KCategoryDrawerV2
{
public:
    CategoryDrawer();

    virtual void drawCategory(const QModelIndex &index,
                              int sortRole,
                              const QStyleOption &option,
                              QPainter *painter) const;

    virtual int categoryHeight(const QModelIndex &index, const QStyleOption &option) const;
};

#endif