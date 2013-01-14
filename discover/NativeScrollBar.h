/*
 *   Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef NATIVESCROLLBAR_H
#define NATIVESCROLLBAR_H

#include <QtDeclarative/QDeclarativeItem>

class QScrollBar;
class NativeScrollBar : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    public:
        NativeScrollBar(QDeclarativeItem* parent = 0);
        virtual ~NativeScrollBar();

        int maximum() const;
        void setMaximum(int max);

        int minimum() const;
        void setMinimum(int min);

        int value() const;
        void setValue(int val);

        Qt::Orientation orientation() const;
        void setOrientation(Qt::Orientation orientation);

        virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);

    Q_SIGNALS:
        void valueChanged(int value);

    private:
        QScrollBar* m_scrollBar;
        QGraphicsProxyWidget* m_proxy;
};

#endif // NATIVESCROLLBAR_H
