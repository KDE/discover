/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ProgressWidget.h"

// Qt includes
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QDebug>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

// KDE includes
#include <KFormat>
#include <KMessageBox>
#include <klocalizedstring.h>

#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesUpdatesModel.h>
#include "ui_ProgressWidget.h"

ProgressWidget::ProgressWidget(ResourcesUpdatesModel* updates, QWidget *parent)
    : QWidget(parent)
    , m_updater(updates)
    , m_lastRealProgress(0)
    , m_show(false)
    , m_ui(new Ui::ProgressWidget)
{
    m_ui->setupUi(this);
    int finalHeight = sizeHint().height() + 20;

    QPropertyAnimation *anim1 = new QPropertyAnimation(this, "maximumHeight", this);
    anim1->setDuration(500);
    anim1->setEasingCurve(QEasingCurve::OutQuart);
    anim1->setStartValue(0);
    anim1->setEndValue(finalHeight);

    QPropertyAnimation *anim2 = new QPropertyAnimation(this, "minimumHeight", this);
    anim2->setDuration(500);
    anim2->setEasingCurve(QEasingCurve::OutQuart);
    anim2->setStartValue(0);
    anim2->setEndValue(finalHeight);

    m_expandWidget = new QParallelAnimationGroup(this);
    m_expandWidget->addAnimation(anim1);
    m_expandWidget->addAnimation(anim2);

    QFont f;
    f.setPointSizeF(f.pointSizeF()*1.5);
    f.setBold(true);
    m_ui->header->setFont(f);

    // Connect the transaction all up to our slots
    connect(m_updater, SIGNAL(progressChanged()),
            this, SLOT(updateProgress()));
    connect(m_updater, SIGNAL(downloadSpeedChanged()),
            this, SLOT(downloadSpeedChanged()));
    connect(m_updater, SIGNAL(etaChanged()), SLOT(etaChanged()));
    connect(m_updater, SIGNAL(cancelableChanged()), SLOT(cancelChanged()));
    connect(m_updater, SIGNAL(statusMessageChanged(QString)),
            m_ui->header, SLOT(setText(QString)));
    connect(m_updater, SIGNAL(statusDetailChanged(QString)),
            m_ui->details, SLOT(setText(QString)));
    connect(m_updater, SIGNAL(progressingChanged()),
            SLOT(updateIsProgressing()));
    
    cancelChanged();
    connect(m_ui->cancel, SIGNAL(clicked()), m_updater, SLOT(cancel()));
}

ProgressWidget::~ProgressWidget()
{
    delete m_ui;
}

void ProgressWidget::updateIsProgressing()
{
    m_ui->progress->setMaximum(m_updater->isProgressing() ? 100 : 0);
}

void ProgressWidget::updateProgress()
{
    qreal progress = m_updater->progress();
    if (progress > 100 || progress<0) {
        m_ui->progress->setMaximum(0);
    } else if (progress > m_lastRealProgress) {
        m_ui->progress->setMaximum(100);
        m_ui->progress->setValue(progress);
        m_lastRealProgress = progress;
    }
}

void ProgressWidget::downloadSpeedChanged()
{
    quint64 speed = m_updater->downloadSpeed();
    if(speed>0) {
        QString downloadSpeed = i18nc("@label Download rate", "Download rate: %1/s",
                                KFormat().formatByteSize(speed));
        m_ui->downloadSpeed->setText(downloadSpeed);
        m_ui->downloadSpeed->show();
    } else {
        m_ui->downloadSpeed->clear();
        m_ui->downloadSpeed->hide();
    }
}

void ProgressWidget::etaChanged()
{
    m_ui->details->setText(m_updater->remainingTime());
}

void ProgressWidget::show()
{
    QWidget::show();

    if (!m_show) {
        m_show = true;
        // Disconnect from previous animatedHide(), else we'll hide once we finish showing
        disconnect(m_expandWidget, SIGNAL(finished()), this, SLOT(hide()));
        m_expandWidget->setDirection(QAbstractAnimation::Forward);
        m_expandWidget->start();
    }
}

void ProgressWidget::animatedHide()
{
    m_show = false;

    m_expandWidget->setDirection(QAbstractAnimation::Backward);
    m_expandWidget->start();
    connect(m_expandWidget, SIGNAL(finished()), this, SLOT(hide()));
    connect(m_expandWidget, SIGNAL(finished()), m_ui->cancel, SLOT(show()));
}

void ProgressWidget::cancelChanged()
{
    m_ui->cancel->setEnabled(m_updater->isCancelable());
}
