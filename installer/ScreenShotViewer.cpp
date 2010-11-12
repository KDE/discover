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

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <KTemporaryFile>
#include <KPixmapSequence>
#include <KIO/Job>

#include <KIcon>
#include <KLocale>

#include "ClickableLabel.h"

ScreenShotViewer::ScreenShotViewer(const QString &url, QWidget *parent)
 : QScrollArea(parent)
{
    m_screenshotL = new ClickableLabel(this);
    m_screenshotL->setCursor(Qt::PointingHandCursor);
    m_screenshotL->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_screenshotL->resize(250, 200);
    resize(250, 200);

    setFrameShape(NoFrame);
    setFrameShadow(Plain);
    setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    setWidget(m_screenshotL);
    setWindowIcon(KIcon("layer-visible-on"));

    KTemporaryFile *tempFile = new KTemporaryFile;
    tempFile->setPrefix("appgetfull");
    tempFile->setSuffix(".png");
    tempFile->open();
    KIO::FileCopyJob *job = KIO::file_copy(url,
                                            tempFile->fileName(),
                                            -1,
                                            KIO::Overwrite | KIO::HideProgressInfo);
    connect(job, SIGNAL(result(KJob *)),
            this, SLOT(resultJob(KJob *)));

    m_busySeq = new KPixmapSequenceOverlayPainter(this);
    m_busySeq->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busySeq->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busySeq->setWidget(m_screenshotL);
    m_busySeq->start();

    connect(m_screenshotL, SIGNAL(clicked()), this, SLOT(deleteLater()));
}

ScreenShotViewer::~ScreenShotViewer()
{
}

void ScreenShotViewer::resultJob(KJob *job)
{
    m_busySeq->stop();
    KIO::FileCopyJob *fJob = qobject_cast<KIO::FileCopyJob*>(job);
    if (!fJob->error()) {
        m_screenshot = QPixmap(fJob->destUrl().toLocalFile());

        QPropertyAnimation *anim1 = new QPropertyAnimation(this, "size");
        anim1->setDuration(500);
        anim1->setStartValue(size());
        anim1->setEndValue(m_screenshot.size());
        anim1->setEasingCurve(QEasingCurve::OutCubic);

        connect(anim1, SIGNAL(finished()), this, SLOT(fadeIn()));
        anim1->start();
    } else {
        m_screenshotL->setText(i18n("Could not find screen shot."));
    }
}

void ScreenShotViewer::fadeIn()
{
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(m_screenshotL);
    effect->setOpacity(0);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(500);
    anim->setStartValue(qreal(0));
    anim->setEndValue(qreal(1));
    m_screenshotL->setGraphicsEffect(effect);
    m_screenshotL->setPixmap(m_screenshot);
    m_screenshotL->adjustSize();

    anim->start();
}

#include "ScreenShotViewer.moc"
