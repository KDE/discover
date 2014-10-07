/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef SCREENSHOTOVERLAY_H
#define SCREENSHOTOVERLAY_H

#include <QtCore/QPointer>
#include <QtWidgets/QWidget>
#include <QPixmap>

class QMouseEvent;
class QScrollArea;

class ClickableLabel;

/**
 * Class to display a screenshot as an overlay of a widget
 */
class ScreenShotOverlay : public QWidget
{
  Q_OBJECT
  public:
    /**
     * Create an overlay widget on @p baseWidget for the image at @p url.
     * @p baseWidget must not be null.
     * @p parent must not be equal to @p baseWidget
     */
    explicit ScreenShotOverlay(const QString &url, QWidget *baseWidget, QWidget *parent = 0);
    ~ScreenShotOverlay();

  protected:
    bool eventFilter( QObject *object, QEvent *event );
    virtual void mousePressEvent(QMouseEvent *event);

  private slots:
    void fadeIn();

  private:
    QPointer<QWidget> mBaseWidget;
    QScrollArea *m_scrollArea;
    QPixmap m_screenshot;
    ClickableLabel *m_screenshotL;

    void reposition();

  signals:
    void clicked();
};

#endif
