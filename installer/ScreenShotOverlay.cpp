/*
    This file is part of Akonadi Contact.

    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "ScreenShotOverlay.h"

#include <QApplication>
#include <QtCore/QEvent>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QProgressBar>

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QtGui/QScrollArea>

#include <KIcon>
#include <KLocale>

#include "ClickableLabel.h"
#include "effects/GraphicsOpacityDropShadowEffect.h"

#define BLUR_RADIUS 15

ScreenShotOverlay::ScreenShotOverlay(const QString &url, QWidget *baseWidget, QWidget *parent)
  : QWidget( parent ? parent : baseWidget->window() ),
    mBaseWidget( baseWidget )
{
  connect( baseWidget, SIGNAL( destroyed() ), SLOT( deleteLater() ) );
  setAttribute(Qt::WA_DeleteOnClose);
  setCursor(Qt::PointingHandCursor);

  QBoxLayout *topLayout = new QVBoxLayout( this );
  QWidget *topStretch = new QWidget(this);
  topStretch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  topLayout->addWidget(topStretch);

  m_scrollArea = new QScrollArea(this);

  m_screenshotL = new ClickableLabel(m_scrollArea);
  m_screenshotL->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  m_screenshotL->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  m_screenshotL->setCursor(Qt::PointingHandCursor);

  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setFrameShadow(QFrame::Plain);
  m_scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  m_scrollArea->setWidget(m_screenshotL);
  m_screenshotL->resize(250, 500);
  m_scrollArea->resize(250, 500);

  m_screenshot = QPixmap(url);

  connect(m_screenshotL, SIGNAL(clicked()), this, SLOT(close()));
  connect(this, SIGNAL(clicked()), this, SLOT(close()));

  topLayout->addWidget(m_scrollArea);
  QWidget *bottomStretch = new QWidget(this);
  bottomStretch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  topLayout->addWidget(bottomStretch);

  QPalette p = palette();
  p.setColor( backgroundRole(), QColor( 0, 0, 0, 128 ) );
  setPalette( p );
  setAutoFillBackground( true );

  mBaseWidget->installEventFilter( this );

  fadeIn();

  reposition();
}

ScreenShotOverlay::~ ScreenShotOverlay()
{
}

void ScreenShotOverlay::reposition()
{
  if ( !mBaseWidget )
    return;

  // reparent to the current top level widget of the base widget if needed
  // needed eg. in dock widgets
  if ( parentWidget() != mBaseWidget->window() )
    //setParent( mBaseWidget->window() );

  // follow base widget visibility
  // needed eg. in tab widgets
  if ( !mBaseWidget->isVisible() ) {
    hide();
    return;
  }
  show();

  // follow position changes
  const QPoint topLevelPos = mBaseWidget->mapTo( window(), QPoint( 0, 0 ) );
  const QPoint parentPos = parentWidget()->mapFrom( window(), topLevelPos );
  move( parentPos );

  // follow size changes
  // TODO: hide/scale icon if we don't have enough space
  resize( mBaseWidget->size() );
}

void ScreenShotOverlay::fadeIn()
{
    GraphicsOpacityDropShadowEffect *effect = new GraphicsOpacityDropShadowEffect(m_screenshotL);
    effect->setBlurRadius(BLUR_RADIUS);
    effect->setOpacity(0);
    effect->setOffset(2);
    effect->setColor(QApplication::palette().dark().color());

    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(qreal(0));
    anim->setEndValue(qreal(1));

    m_screenshotL->setGraphicsEffect(effect);
    m_screenshotL->setPixmap(m_screenshot);
    m_screenshotL->adjustSize();

    anim->start();
}

bool ScreenShotOverlay::eventFilter(QObject * object, QEvent * event)
{
  if ( object == mBaseWidget &&
    ( event->type() == QEvent::Move || event->type() == QEvent::Resize ||
      event->type() == QEvent::Show || event->type() == QEvent::Hide ||
      event->type() == QEvent::ParentChange ) ) {
    reposition();
  }
  return QWidget::eventFilter( object, event );
}

void ScreenShotOverlay::mousePressEvent(QMouseEvent *event)
{
    emit clicked();
    event->accept();
}


#include "ScreenShotOverlay.moc"
