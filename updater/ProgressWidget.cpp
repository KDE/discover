#include "ProgressWidget.h"

// Qt includes
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KGlobal>
#include <KIcon>
#include <KLocale>

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);

    m_statusLabel = new QLabel(this);
    m_progressBar = new QProgressBar(this);

    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    widget->setLayout(layout);

    QPushButton *cancelButton = new QPushButton(widget);
    cancelButton->setText(i18nc("@action:button Cancels the download", "Cancel"));
    cancelButton->setIcon(KIcon("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SIGNAL(cancelDownload()));

    layout->addWidget(m_progressBar);
    layout->addWidget(cancelButton);

    m_detailsLabel = new QLabel(this);

    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(m_detailsLabel);

    setLayout(mainLayout);

    int finalHeight = sizeHint().height() + 20;

    QPropertyAnimation *anim1 = new QPropertyAnimation(this, "maximumSize", this);
    anim1->setDuration(500);
    anim1->setEasingCurve(QEasingCurve::OutQuart);
    anim1->setStartValue(QSize(QWIDGETSIZE_MAX, 0));
    anim1->setEndValue(QSize(QWIDGETSIZE_MAX, finalHeight));
    QPropertyAnimation *anim2 = new QPropertyAnimation(this, "minimumSize", this);
    anim2->setDuration(500);
    anim2->setEasingCurve(QEasingCurve::OutQuart);
    anim2->setStartValue(QSize(QWIDGETSIZE_MAX, 0));
    anim2->setEndValue(QSize(QWIDGETSIZE_MAX, finalHeight));

    m_expandWidget = new QParallelAnimationGroup(this);
    m_expandWidget->addAnimation(anim1);
    m_expandWidget->addAnimation(anim2);
}

void ProgressWidget::setCommitProgress(int percentage)
{
    m_progressBar->setValue(percentage);
}

void ProgressWidget::updateDownloadProgress(int percentage, int speed, int ETA)
{
    m_progressBar->setValue(percentage);

    QString downloadSpeed;
    if (speed > -1) {
        downloadSpeed = i18nc("@label Download rate", "Download rate: %1/s",
                              KGlobal::locale()->formatByteSize(speed));
    }

    QString timeRemaining;
    int ETAMilliseconds = ETA * 1000;

    if (ETAMilliseconds > 0 && ETAMilliseconds < 14 * 24 * 60 * 60) {
        // If ETA is greater than zero or less than 2 weeks
        timeRemaining = i18nc("@item:intext Remaining time", "%1 remaining",
                              KGlobal::locale()->prettyFormatDuration(ETAMilliseconds));
    }

    QString label = downloadSpeed;

    if (!label.isEmpty() && !timeRemaining.isEmpty()) {
        label.append(QLatin1String(" - ") % timeRemaining);
    } else {
        label = timeRemaining;
    }

    m_detailsLabel->setText(label);
}

void ProgressWidget::updateCommitProgress(const QString &message, int percentage)
{
    Q_UNUSED(message);
    m_progressBar->setValue(percentage);
}

void ProgressWidget::setHeaderText(const QString &text)
{
    m_statusLabel->setText(text);
}

void ProgressWidget::show()
{
    QWidget::show();

    m_expandWidget->setDirection(QAbstractAnimation::Forward);
    m_expandWidget->start();
}

void ProgressWidget::animatedHide()
{
    m_expandWidget->setDirection(QAbstractAnimation::Backward);
    m_expandWidget->start();
    connect(m_expandWidget, SIGNAL(finished()), this, SLOT(hide()));
}
