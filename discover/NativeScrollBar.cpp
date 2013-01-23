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

#include "NativeScrollBar.h"
#include <QScrollBar>
#include <QGraphicsProxyWidget>

NativeScrollBar::NativeScrollBar(QDeclarativeItem* parent)
    : QDeclarativeItem(parent)
{
    m_scrollBar = new QScrollBar;
    m_scrollBar->setAttribute(Qt::WA_NoSystemBackground);
    connect(m_scrollBar, SIGNAL(sliderMoved(int)), SIGNAL(valueChanged(int)));
    m_proxy = new QGraphicsProxyWidget(this);
    m_proxy->setWidget(m_scrollBar);

    setImplicitWidth(m_scrollBar->sizeHint().width());
}

NativeScrollBar::~NativeScrollBar()
{
    delete m_scrollBar;
}

void NativeScrollBar::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
    m_proxy->setGeometry(QRectF(QPointF(0,0), newGeometry.size()));
}

int NativeScrollBar::minimum() const { return m_scrollBar->minimum(); }
int NativeScrollBar::maximum() const { return m_scrollBar->maximum(); }
int NativeScrollBar::pageStep() const { return m_scrollBar->pageStep(); }
Qt::Orientation NativeScrollBar::orientation() const { return m_scrollBar->orientation(); }
int NativeScrollBar::value() const { return m_scrollBar->value(); }
void NativeScrollBar::setMaximum(int max) { m_scrollBar->setMaximum(max); emit maximumChanged(); }
void NativeScrollBar::setMinimum(int min) { m_scrollBar->setMinimum(min); emit minimumChanged(); }
void NativeScrollBar::setPageStep(int pageStep) { m_scrollBar->setPageStep(pageStep); emit pageStepChanged(); }
void NativeScrollBar::setOrientation(Qt::Orientation orientation) { m_scrollBar->setOrientation(orientation); }
void NativeScrollBar::setValue(int val) { m_scrollBar->setValue(val); }
