/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "DownloadWidget.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QStandardItemModel>

// KDE includes
#include <KGlobal>
#include <KHBox>
#include <KIcon>
#include <KLocale>

// LibQApt includes
#include <libqapt/globals.h>

DownloadWidget::DownloadWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    m_headerLabel = new QLabel(this);
    layout->addWidget(m_headerLabel);

    m_downloadModel = new QStandardItemModel(this);

    //TODO: Fancy model/view with progress bars for each item
    m_downloadView = new QListView(this);
    layout->addWidget(m_downloadView);
    m_downloadView->setModel(m_downloadModel);

    m_downloadLabel = new QLabel(this);
    layout->addWidget(m_downloadLabel);

    KHBox *hbox = new KHBox(this);
    layout->addWidget(hbox);
    m_totalProgress = new QProgressBar(hbox);

    m_cancelButton = new QPushButton(hbox);
    m_cancelButton->setText(i18nc("@action:button Cancels the download", "Cancel"));
    m_cancelButton->setIcon(KIcon("dialog-cancel"));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonPressed()));
}

DownloadWidget::~DownloadWidget()
{
}

void DownloadWidget::clear()
{
    m_downloadModel->clear();
    m_totalProgress->setValue(0);
}

void DownloadWidget::setHeaderText(const QString &text)
{
    m_headerLabel->setText(text);
}

void DownloadWidget::addItem(const QString &message)
{
    QStandardItem *n = new QStandardItem();
    n->setText(message);
    m_downloadModel->appendRow(n);
    m_downloadView->scrollTo(m_downloadModel->indexFromItem(n));
}

void DownloadWidget::updateDownloadProgress(int percentage, int speed, int ETA)
{
    m_totalProgress->setValue(percentage);

    QString downloadSpeed;
    if (speed < 0) {
        downloadSpeed = i18nc("@label Label for when the download speed is unknown", "Unknown speed");
    } else {
        downloadSpeed = i18nc("@label Download rate", "Download rate: %1/s",
                              KGlobal::locale()->formatByteSize(speed));
    }

    QString timeRemaining;
    int ETAMilliseconds = ETA * 1000;

    if (ETAMilliseconds <= 0 || ETAMilliseconds > 14*24*60*60) {
        // If ETA is less than zero or bigger than 2 weeks
        timeRemaining = i18nc("@item:intext Label for when the remaining time is unknown",
                              " - Unknown time remaining");
    } else {
        timeRemaining = i18nc("@item:intext Remaining time", " - %1 remaining",
                              KGlobal::locale()->prettyFormatDuration(ETAMilliseconds));
    }
    m_downloadLabel->setText(downloadSpeed + timeRemaining);
}

void DownloadWidget::updateDownloadMessage(int flag, const QString &message)
{
    QString fullMessage;

    switch (flag) {
      case QApt::DownloadFetch:
          fullMessage = i18nc("@info:status", "Downloading: %1", message);
          break;
      case QApt::HitFetch:
          fullMessage = i18nc("@info:status", "Checking: %1", message);
          break;
      case QApt::IgnoredFetch:
          fullMessage = i18nc("@info:status", "Ignored: %1", message);
          break;
      default:
          fullMessage = message;
    }
    addItem(fullMessage);
}

void DownloadWidget::cancelButtonPressed()
{
    emit cancelDownload();
}

#include "DownloadWidget.moc"
