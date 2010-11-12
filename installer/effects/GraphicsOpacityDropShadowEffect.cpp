/***************************************************************************
 *   Copyright (C) 2010 by Daniel Nicoletti                                *
 *   dantti85-pk@yahoo.com.br                                              *
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
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include "GraphicsOpacityDropShadowEffect.h"

#include <QPainter>
#include <QVariant>

GraphicsOpacityDropShadowEffect::GraphicsOpacityDropShadowEffect(QObject *parent)
 : QGraphicsDropShadowEffect(parent),
   m_opacity(1)
{
}

GraphicsOpacityDropShadowEffect::~GraphicsOpacityDropShadowEffect()
{
}

qreal GraphicsOpacityDropShadowEffect::opacity() const
{
    return m_opacity;
}

void  GraphicsOpacityDropShadowEffect::setOpacity(qreal opacity)
{
    m_opacity = opacity;
    update();
}

void GraphicsOpacityDropShadowEffect::draw(QPainter *painter)
{
    painter->setOpacity(m_opacity);
    QGraphicsDropShadowEffect::draw(painter);
}

#include "GraphicsOpacityDropShadowEffect.moc"
