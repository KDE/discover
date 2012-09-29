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
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KGlobal>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>

// LibQApt includes
#include <LibQApt/Transaction>

// Own includes
#include "MuonStrings.h"

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_trans(nullptr)
    , m_lastRealProgress(0)
    , m_show(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);

    m_headerLabel = new QLabel(this);
    m_progressBar = new QProgressBar(this);

    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    widget->setLayout(layout);

    m_cancelButton = new QPushButton(widget);
    m_cancelButton->setText(i18nc("@action:button Cancels the download", "Cancel"));
    m_cancelButton->setIcon(KIcon("dialog-cancel"));

    layout->addWidget(m_progressBar);
    layout->addWidget(m_cancelButton);

    m_detailsLabel = new QLabel(this);

    mainLayout->addWidget(m_headerLabel);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(m_detailsLabel);

    setLayout(mainLayout);

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
}

void ProgressWidget::setTransaction(QApt::Transaction *trans)
{
    m_trans = trans;

    // Connect the transaction all up to our slots
    connect(m_trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(statusChanged(QApt::TransactionStatus)));
    connect(m_trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            this, SLOT(transactionErrorOccurred(QApt::ErrorCode)));
    connect(m_trans, SIGNAL(cancellableChanged(bool)),
            m_cancelButton, SLOT(setVisible(bool)));
    connect(m_trans, SIGNAL(mediumRequired(QString,QString)),
            this, SLOT(provideMedium(QString,QString)));
    connect(m_trans, SIGNAL(promptUntrusted(QStringList)),
            this, SLOT(untrustedPrompt(QStringList)));
    connect(m_trans, SIGNAL(progressChanged(int)),
            this, SLOT(updateProgress(int)));
    connect(m_trans, SIGNAL(downloadSpeedChanged(quint64)),
            this, SLOT(downloadSpeedChanged(quint64)));

    // Connect us to the transaction
    connect(m_cancelButton, SIGNAL(clicked()), m_trans, SLOT(cancel()));
    statusChanged(m_trans->status());
}

void ProgressWidget::statusChanged(QApt::TransactionStatus status)
{
    switch (status) {
    case QApt::SetupStatus:
        m_headerLabel->setText(i18nc("@info Status info, widget title",
                                     "<title>Starting</title>"));
        m_progressBar->setMaximum(0);
        break;
    case QApt::AuthenticationStatus:
        m_headerLabel->setText(i18nc("@info Status info, widget title",
                                     "<title>Waiting for Authentication</title>"));
        m_progressBar->setMaximum(0);
        break;
    case QApt::WaitingStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_detailsLabel->setText(i18nc("@info Status info",
                                     "Waiting for other transactions to finish"));
        m_progressBar->setMaximum(0);
        break;
    case QApt::WaitingLockStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_detailsLabel->setText(i18nc("@info Status info",
                                      "Waiting for other software managers to quit"));
        m_progressBar->setMaximum(0);
        break;
    case QApt::WaitingMediumStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_detailsLabel->setText(i18nc("@info Status info",
                                     "Waiting for required medium"));
        m_progressBar->setMaximum(0);
        break;
    case QApt::RunningStatus:
        m_progressBar->setMaximum(100);
        m_headerLabel->clear();
        m_detailsLabel->clear();
        break;
    case QApt::LoadingCacheStatus:
        m_detailsLabel->clear();
        m_headerLabel->setText(i18nc("@info Status info",
                                     "<title>Loading Software List</title>"));
        break;
    case QApt::DownloadingStatus:
        switch (m_trans->role()) {
        case QApt::UpdateCacheRole:
            m_headerLabel->setText(i18nc("@info Status information, widget title",
                                         "<title>Updating software sources</title>"));
            break;
        case QApt::DownloadArchivesRole:
        case QApt::CommitChangesRole:
            m_headerLabel->setText(i18nc("@info Status information, widget title",
                                         "<title>Downloading Packages</title>"));
            break;
        default:
            break;
        }
        break;
    case QApt::CommittingStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Applying Changes</title>"));
        m_detailsLabel->clear();
        break;
    case QApt::FinishedStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Finished</title>"));
    }
}

void ProgressWidget::transactionErrorOccurred(QApt::ErrorCode error)
{
    if (error == QApt::Success)
        return;

    MuonStrings *muonStrings = MuonStrings::global();

    QString title = muonStrings->errorTitle(error);
    QString text = muonStrings->errorText(error, m_trans);

    switch (error) {
    case QApt::InitError:
    case QApt::FetchError:
    case QApt::CommitError:
        KMessageBox::detailedError(this, text, m_trans->errorDetails(), title);
        break;
    default:
        KMessageBox::error(this, text, title);
        break;
    }
}

void ProgressWidget::provideMedium(const QString &label, const QString &medium)
{
    QString title = i18nc("@title:window", "Media Change Required");
    QString text = i18nc("@label", "Please insert %1 into <filename>%2</filename>",
                         label, medium);

    KMessageBox::information(this, text, title);
    m_trans->provideMedium(medium);
}

void ProgressWidget::untrustedPrompt(const QStringList &untrustedPackages)
{
    QString title = i18nc("@title:window", "Warning - Unverified Software");
    QString text = i18ncp("@label",
                          "The following piece of software cannot be verified. "
                          "<warning>Installing unverified software represents a "
                          "security risk, as the presence of unverifiable software "
                          "can be a sign of tampering.</warning> Do you wish to continue?",
                          "The following pieces of software cannot be authenticated. "
                          "<warning>Installing unverified software represents a "
                          "security risk, as the presence of unverifiable software "
                          "can be a sign of tampering.</warning> Do you wish to continue?",
                          untrustedPackages.size());
    int result = KMessageBox::warningContinueCancelList(this, text,
                                                    untrustedPackages, title);

    bool installUntrusted = (result == KMessageBox::Continue);
    m_trans->replyUntrustedPrompt(installUntrusted);
}

void ProgressWidget::updateProgress(int progress)
{
    if (progress > 100) {
        m_progressBar->setMaximum(0);
    } else if (progress > m_lastRealProgress) {
        m_progressBar->setMaximum(100);
        m_progressBar->setValue(progress);
        m_lastRealProgress = progress;
    }
}

void ProgressWidget::downloadSpeedChanged(quint64 speed)
{
    QString downloadSpeed = i18nc("@label Download rate", "Download rate: %1/s",
                              KGlobal::locale()->formatByteSize(speed));
    m_detailsLabel->setText(downloadSpeed);
}

void ProgressWidget::etaChanged(quint64 ETA)
{
    QString label = m_detailsLabel->text();

    QString timeRemaining;
    // Ignore ETA if it's larger than 2 days.
    if (ETA && ETA < 2 * 24 * 60 * 60) {
        timeRemaining = i18nc("@item:intext Remaining time", "%1 remaining",
                              KGlobal::locale()->prettyFormatDuration(ETA * 1000));
    }

    if (!timeRemaining.isEmpty()) {
        label.append(QLatin1String(" - ") % timeRemaining);
        m_detailsLabel->setText(label);
    }
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
    connect(m_expandWidget, SIGNAL(finished()), m_cancelButton, SLOT(show()));
}
