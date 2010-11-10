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

#include "CategoryDrawer.h"

#include <KCategorizedSortFilterProxyModel>

#include <QPainter>
#include <QApplication>
#include <QStyleOption>

CategoryDrawer::CategoryDrawer()
{
    setLeftMargin( 7 );
    setRightMargin( 7 );
}

void CategoryDrawer::drawCategory(const QModelIndex &index,
                                            int sortRole,
                                            const QStyleOption &option,
                                            QPainter *painter) const
{
    Q_UNUSED( option )
    Q_UNUSED( painter )
    Q_UNUSED( sortRole )

    painter->setRenderHint(QPainter::Antialiasing);

    const QRect optRect = option.rect;
    QFont font(QApplication::font());
    font.setBold(true);
    const QFontMetrics fontMetrics = QFontMetrics(font);
    const int height = categoryHeight(index, option);

    //BEGIN: decoration gradient
    {
        QPainterPath path(optRect.bottomLeft());
        path.lineTo(QPoint(optRect.topLeft().x(), optRect.topLeft().y() - 3));
        const QPointF topLeft(optRect.topLeft());
        QRectF arc(topLeft, QSizeF(4, 4));
        path.arcTo(arc, 180, -90);
        path.lineTo(optRect.topRight());
        path.lineTo(optRect.bottomRight());
        path.lineTo(optRect.bottomLeft());

        QColor window(option.palette.window().color());
        const QColor base(option.palette.base().color());

        window.setAlphaF(0.4);

        QLinearGradient decoGradient1(optRect.topLeft(), optRect.bottomLeft());
        decoGradient1.setColorAt(0, window);
        decoGradient1.setColorAt(1, Qt::transparent);

        QLinearGradient decoGradient2(optRect.topLeft(), optRect.topRight());
        decoGradient2.setColorAt(0, Qt::transparent);
        decoGradient2.setColorAt(1, base);

        painter->fillPath(path, decoGradient1);
        painter->fillPath(path, decoGradient2);
    }
    //END: decoration gradient

    {
        QRect newOptRect(optRect);
        newOptRect.setLeft(newOptRect.left() + 1);
        newOptRect.setTop(newOptRect.top() + 1);

        //BEGIN: inner top left corner
        {
            painter->save();
            painter->setPen(option.palette.base().color());
            const QPointF topLeft(newOptRect.topLeft());
            QRectF arc(topLeft, QSizeF(4, 4));
            arc.translate(0.5, 0.5);
            painter->drawArc(arc, 1440, 1440);
            painter->restore();
        }
        //END: inner top left corner

        //BEGIN: inner left vertical line
        {
            QPoint start(newOptRect.topLeft());
            start.ry() += 3;
            QPoint verticalGradBottom(newOptRect.topLeft());
            verticalGradBottom.ry() += newOptRect.height() - 3;
            QLinearGradient gradient(start, verticalGradBottom);
            gradient.setColorAt(0, option.palette.base().color());
            gradient.setColorAt(1, Qt::transparent);
            painter->fillRect(QRect(start, QSize(1, newOptRect.height() - 3)), gradient);
        }
        //END: inner left vertical line

        //BEGIN: inner horizontal line
        {
            QPoint start(newOptRect.topLeft());
            start.rx() += 3;
            QPoint horizontalGradTop(newOptRect.topLeft());
            horizontalGradTop.rx() += newOptRect.width() - 3;
            QLinearGradient gradient(start, horizontalGradTop);
            gradient.setColorAt(0, option.palette.base().color());
            gradient.setColorAt(1, Qt::transparent);
            painter->fillRect(QRect(start, QSize(newOptRect.width() - 3, 1)), gradient);
        }
        //END: inner horizontal line
    }

    QColor outlineColor = option.palette.text().color();
    outlineColor.setAlphaF(0.35);

    //BEGIN: top left corner
    {
        painter->save();
        painter->setPen(outlineColor);
        const QPointF topLeft(optRect.topLeft());
        QRectF arc(topLeft, QSizeF(4, 4));
        arc.translate(0.5, 0.5);
        painter->drawArc(arc, 1440, 1440);
        painter->restore();
    }
    //END: top left corner

    //BEGIN: left vertical line
    {
        QPoint start(optRect.topLeft());
        start.ry() += 3;
        QPoint verticalGradBottom(optRect.topLeft());
        verticalGradBottom.ry() += optRect.height() - 3;
        QLinearGradient gradient(start, verticalGradBottom);
        gradient.setColorAt(0, outlineColor);
        gradient.setColorAt(1, option.palette.base().color());
        painter->fillRect(QRect(start, QSize(1, optRect.height() - 3)), gradient);
    }
    //END: left vertical line

    //BEGIN: horizontal line
    {
        QPoint start(optRect.topLeft());
        start.rx() += 3;
        QPoint horizontalGradTop(optRect.topLeft());
        horizontalGradTop.rx() += optRect.width() - 3;
        QLinearGradient gradient(start, horizontalGradTop);
        gradient.setColorAt(0, outlineColor);
        gradient.setColorAt(1, option.palette.base().color());
        painter->fillRect(QRect(start, QSize(optRect.width() - 3, 1)), gradient);
    }
    //END: horizontal line

    //BEGIN: draw text
    {
        const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
        QRect textRect = QRect(option.rect.topLeft(), QSize(option.rect.width(), height));
        textRect.setTop(textRect.top() + 2 + 3 /* corner */);
        textRect.setLeft(textRect.left() + 2 + 3 /* corner */ + 3 /* a bit of margin */);
        painter->save();
        painter->setFont(font);
        QColor penColor(option.palette.text().color());
        penColor.setAlphaF(0.6);
        painter->setPen(penColor);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, category);
        painter->restore();
    }
    //END: draw text
}

int CategoryDrawer::categoryHeight(const QModelIndex &index, const QStyleOption &option) const
{
    Q_UNUSED( index );
    Q_UNUSED( option );

    QFont font(QApplication::font());
    font.setBold(true);
    const QFontMetrics fontMetrics = QFontMetrics(font);

    return fontMetrics.height() + 2 + 12 /* vertical spacing */;
}
