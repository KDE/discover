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

#include "ApplicationExtender.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>

#include <KDebug>
#include <KIcon>
#include <KLocale>
#include <KStandardGuiItem>

#include <LibQApt/Package>

#include "Application.h"

ApplicationExtender::ApplicationExtender(QWidget *parent, Application *app, ApplicationBackend *backend)
    : QWidget(parent)
    , m_app(app)
    , m_appBackend(backend)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    QPushButton *infoButton = new QPushButton(this);
    infoButton->setText(i18n("More Info"));
    connect(infoButton, SIGNAL(clicked()), this, SLOT(emitInfoButtonClicked()));

    QWidget *buttonSpacer = new QWidget(this);
    buttonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_actionButton = new QPushButton(this);

    if (app->package()->isInstalled()) {
        m_actionButton->setIcon(KIcon("edit-delete"));
        m_actionButton->setText(i18n("Remove"));
        connect(m_actionButton, SIGNAL(clicked()), this, SLOT(emitRemoveButtonClicked()));
    } else {
        m_actionButton->setIcon(KIcon("download"));
        m_actionButton->setText(i18n("Install"));
        connect(m_actionButton, SIGNAL(clicked()), this, SLOT(emitInstallButtonClicked()));
    }

    m_progressBar = new QProgressBar(this);
    m_progressBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_progressBar->hide();

    m_cancelButton = new QPushButton(this);
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    m_cancelButton->setIcon(cancelButton.icon());
    m_cancelButton->setToolTip(cancelButton.toolTip());
    m_cancelButton->hide();
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(emitCancelButtonClicked()));

    layout->addWidget(infoButton);
    layout->addWidget(buttonSpacer);
    layout->addWidget(m_actionButton);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_cancelButton);

    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent, Transaction *)),
            this, SLOT(workerEvent(QApt::WorkerEvent, Transaction *)));
    connect(m_appBackend, SIGNAL(transactionCancelled(Application *)),
            this, SLOT(transactionCancelled(Application *)));

    // Catch already-begun downloads. If the state is something else, we won't
    // care because we won't handle it
    QPair<QApt::WorkerEvent, Transaction *> workerState = m_appBackend->workerState();
    workerEvent(workerState.first, workerState.second);

    foreach (Transaction *transaction, m_appBackend->transactions()) {
        if (transaction->application() == m_app){
            showTransactionState(transaction);
        }
    }

}

ApplicationExtender::~ApplicationExtender()
{
}

void ApplicationExtender::workerEvent(QApt::WorkerEvent event, Transaction *transaction)
{
    if (!transaction || m_app != transaction->application()) {
        return;
    }

    switch (event) {
    case QApt::PackageDownloadStarted:
        m_actionButton->hide();
        m_cancelButton->show();
        m_progressBar->show();
        m_progressBar->setFormat(i18nc("@info:status", "Downloading"));
        connect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                   this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::CommitChangesStarted:
        m_actionButton->hide();
        m_cancelButton->hide();
        m_progressBar->show();
        m_progressBar->setValue(0);
        switch (transaction->action()) {
        case InstallApp:
            m_progressBar->setFormat(i18nc("@info:status", "Installing"));
            break;
        case ChangeAddons:
            m_progressBar->setFormat(i18nc("@info:status", "Changing Addons"));
            break;
        case RemoveApp:
            m_progressBar->setFormat(i18nc("@info:status", "Removing"));
            break;
        default:
            break;
        }
        connect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::CommitChangesFinished:
        disconnect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                   this, SLOT(updateProgress(Transaction *, int)));
        break;
    default:
        break;
    }
}

void ApplicationExtender::updateProgress(Transaction *transaction, int percentage)
{
    if (m_app == transaction->application()) {
        m_progressBar->setValue(percentage);

        if (percentage == 100) {
            m_progressBar->setFormat(i18nc("@info:status", "Done"));
        }
    }
}

void ApplicationExtender::showTransactionState(Transaction *transaction)
{
    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);

    QString text;
    switch (transaction->state()) {
    case QueuedState:
        text = i18nc("@info:status", "Waiting");
        break;
    case RunningState:
        switch (transaction->action()) {
        case InstallApp:
            text = i18nc("@info:status", "Installing");
            break;
        case ChangeAddons:
            text = i18nc("@info:status", "Changing Addons");
            break;
        case RemoveApp:
            text = i18nc("@info:status", "Removing");
            break;
        default:
            break;
        }
        break;
    case DoneState:
        text = i18nc("@info:status", "Done");
        m_progressBar->setValue(100);
        break;
    default:
        break;
    }
    m_progressBar->setFormat(text);
}

void ApplicationExtender::transactionCancelled(Application *app)
{
    if (m_app == app) {
        m_progressBar->hide();
        m_cancelButton->hide();
        m_actionButton->show();
    }
}

void ApplicationExtender::emitInfoButtonClicked()
{
    emit infoButtonClicked(m_app);
}

void ApplicationExtender::emitRemoveButtonClicked()
{
    emit removeButtonClicked(m_app);

    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status", "Waiting"));
}

void ApplicationExtender::emitInstallButtonClicked()
{
    emit installButtonClicked(m_app);

    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status", "Waiting"));
}

void ApplicationExtender::emitCancelButtonClicked()
{
    emit cancelButtonClicked(m_app);
}

#include "ApplicationExtender.moc"
