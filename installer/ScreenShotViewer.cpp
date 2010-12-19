/***************************************************************************
 *   Copyright (C) 2009-2010 by Daniel Nicoletti                           *
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

#include "ScreenShotViewer.h"

#include <QApplication>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QtGui/QScrollArea>

#include <KIcon>
#include <KLocale>

#include "ClickableLabel.h"
#include "effects/GraphicsOpacityDropShadowEffect.h"

#define BLUR_RADIUS 15

ScreenShotViewer::ScreenShotViewer(const QString &url, QWidget *parent)
 : KDialog(parent)
{
    m_scrollArea = new QScrollArea(this);
    setMainWidget(m_scrollArea);
    setButtons(KDialog::Close);

    m_screenshotL = new ClickableLabel(m_scrollArea);
    m_screenshotL->setCursor(Qt::PointingHandCursor);
    m_screenshotL->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_screenshotL->resize(250, 200);
    resize(250, 200);

    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setFrameShadow(QFrame::Plain);
    m_scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_scrollArea->setWidget(m_screenshotL);
    m_scrollArea->setWindowIcon(KIcon("layer-visible-on"));

    m_screenshot = QPixmap(url);

    QPropertyAnimation *anim1 = new QPropertyAnimation(this, "size", this);
    anim1->setDuration(500);
    anim1->setStartValue(size());
    anim1->setEndValue(m_screenshot.size() + (size()/3));
    anim1->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim1, SIGNAL(finished()), this, SLOT(fadeIn()));
    anim1->start();

    connect(m_screenshotL, SIGNAL(clicked()), this, SLOT(close()));
}

ScreenShotViewer::~ScreenShotViewer()
{
}

void ScreenShotViewer::fadeIn()
{
    GraphicsOpacityDropShadowEffect *effect = new GraphicsOpacityDropShadowEffect(m_screenshotL);
    effect->setBlurRadius(BLUR_RADIUS);
    effect->setOpacity(0);
    effect->setOffset(2);
    effect->setColor(QApplication::palette().dark().color());

    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(500);
    anim->setStartValue(qreal(0));
    anim->setEndValue(qreal(1));

    m_screenshotL->setGraphicsEffect(effect);
    m_screenshotL->setPixmap(m_screenshot);
    m_screenshotL->adjustSize();

    anim->start();
}

#include "ScreenShotViewer.moc"
