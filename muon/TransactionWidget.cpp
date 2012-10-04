/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "TransactionWidget.h"

// Qt includes
#include <QtCore/QUuid>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QDebug>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KMessageBox>

// LibQApt includes
#include <LibQApt/Transaction>
#include <DebconfGui.h>

// Own includes
#include "../libmuon/MuonStrings.h"
#include "DownloadModel/DownloadDelegate.h"
#include "DownloadModel/DownloadModel.h"

TransactionWidget::TransactionWidget(QWidget *parent)
    : QWidget(parent)
    , m_trans(nullptr)
    , m_lastRealProgress(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    setLayout(layout);

    m_headerLabel = new QLabel(this);
    layout->addWidget(m_headerLabel);

    m_spacer = new QWidget(this);
    m_spacer->hide();
    m_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addWidget(m_spacer);

    m_downloadModel = new DownloadModel(this);
    m_downloadDelegate = new DownloadDelegate(this);

    m_downloadView = new QTreeView(this);
    layout->addWidget(m_downloadView);
    m_downloadView->setModel(m_downloadModel);
    m_downloadView->setRootIsDecorated(false);
    m_downloadView->setUniformRowHeights(true);
    m_downloadView->setItemDelegate(m_downloadDelegate);
    m_downloadView->setSelectionMode(QAbstractItemView::NoSelection);
    m_downloadView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_downloadView->header()->setStretchLastSection(false);
    m_downloadView->header()->setResizeMode(1, QHeaderView::Stretch);
    m_downloadView->hide();

    QString uuid = QUuid::createUuid().toString();
    uuid.remove('{').remove('}').remove('-');
    m_pipe = QLatin1String("/tmp/qapt-sock-") + uuid;

    m_debconfGui = new DebconfKde::DebconfGui(m_pipe, this);
    layout->addWidget(m_debconfGui);
    m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
    m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
    m_debconfGui->hide();

    m_statusLabel = new QLabel(this);
    layout->addWidget(m_statusLabel);

    QWidget *hbox = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout(hbox);
    hboxLayout->setMargin(0);
    hbox->setLayout(hboxLayout);
    layout->addWidget(hbox);
    m_totalProgress = new QProgressBar(hbox);
    hboxLayout->addWidget(m_totalProgress);

    m_cancelButton = new QPushButton(hbox);
    m_cancelButton->setText(i18nc("@action:button Cancels the download", "Cancel"));
    m_cancelButton->setIcon(KIcon("dialog-cancel"));
    hboxLayout->addWidget(m_cancelButton);
    connect(m_downloadModel, SIGNAL(rowsInserted(QModelIndex,int,int)), m_downloadView, SLOT(scrollToBottom()));
}

QString TransactionWidget::pipe() const
{
    return m_pipe;
}

void TransactionWidget::setTransaction(QApt::Transaction *trans)
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
    connect(m_trans, SIGNAL(statusDetailsChanged(QString)),
            m_statusLabel, SLOT(setText(QString)));
    connect(m_trans, SIGNAL(downloadProgressChanged(QApt::DownloadProgress)),
            m_downloadModel, SLOT(updateDetails(QApt::DownloadProgress)));

    // Connect us to the transaction
    connect(m_cancelButton, SIGNAL(clicked()), m_trans, SLOT(cancel()));
    statusChanged(m_trans->status());
}

void TransactionWidget::statusChanged(QApt::TransactionStatus status)
{
    switch (status) {
    case QApt::SetupStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Starting</title>"));
        m_statusLabel->setText(i18nc("@info Status info",
                                     "Waiting for service to start"));
        m_totalProgress->setMaximum(0);
        break;
    case QApt::AuthenticationStatus:
        m_statusLabel->setText(i18nc("@info Status info",
                                     "Waiting for authentication"));
        break;
    case QApt::WaitingStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_statusLabel->setText(i18nc("@info Status info",
                                     "Waiting for other transactions to finish"));
        m_totalProgress->setMaximum(0);
        break;
    case QApt::WaitingLockStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_statusLabel->setText(i18nc("@info Status info",
                                     "Waiting for other software managers to quit"));
        m_totalProgress->setMaximum(0);
        break;
    case QApt::WaitingMediumStatus:
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Waiting</title>"));
        m_statusLabel->setText(i18nc("@info Status info",
                                     "Waiting for required medium"));
        m_totalProgress->setMaximum(0);
        break;
    case QApt::RunningStatus:
        m_totalProgress->setMaximum(100);
        m_headerLabel->clear();
        m_statusLabel->clear();
        break;
    case QApt::LoadingCacheStatus:
        m_statusLabel->clear();
        m_headerLabel->setText(i18nc("@info Status info",
                                     "<title>Loading Software List</title>"));
        break;
    case QApt::DownloadingStatus:
        m_downloadView->show();
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
        m_downloadView->hide();
        m_spacer->show();

        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Committing Changes</title>"));
        break;
    case QApt::FinishedStatus:
        m_spacer->hide();
        m_downloadView->hide();
        m_downloadModel->clear();
        m_headerLabel->setText(i18nc("@info Status information, widget title",
                                     "<title>Finished</title>"));
        m_lastRealProgress = 0;
    }
}

void TransactionWidget::transactionErrorOccurred(QApt::ErrorCode error)
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

void TransactionWidget::provideMedium(const QString &label, const QString &medium)
{
    QString title = i18nc("@title:window", "Media Change Required");
    QString text = i18nc("@label", "Please insert %1 into <filename>%2</filename>",
                         label, medium);

    KMessageBox::information(this, text, title);
    m_trans->provideMedium(medium);
}

void TransactionWidget::untrustedPrompt(const QStringList &untrustedPackages)
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

void TransactionWidget::updateProgress(int progress)
{
    if (progress > 100) {
        m_totalProgress->setMaximum(0);
    } else if (progress > m_lastRealProgress) {
        m_totalProgress->setMaximum(100);
        m_totalProgress->setValue(progress);
        m_lastRealProgress = progress;
    }
}
