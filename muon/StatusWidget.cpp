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

#include "StatusWidget.h"

// Qt includes
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>

// KDE includes
#include <KGlobal>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent)
    , m_backend(0)
{
    m_countsLabel = new QLabel(this);
    m_countsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_downloadLabel = new QLabel(this);
    m_downloadLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    const int fontHeight = QFontMetrics(m_countsLabel->font()).height();

    m_xapianProgress = new QProgressBar(this);
    m_xapianProgress->setMaximumSize(250, fontHeight);
    m_xapianProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_xapianProgress->setFormat(i18nc("@info:status", "Rebuilding Search Index"));
    m_xapianProgress->setTextVisible(true);
    m_xapianProgress->hide();

    m_xapianTimeout = new QTimer(this);
    // Time out if no progress at all is made in 10 seconds
    m_xapianTimeout->setInterval(10000);
    m_xapianTimeout->setSingleShot(true);
    connect(m_xapianTimeout, SIGNAL(timeout()), this, SLOT(hideXapianProgress()));

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    setLayout(topLayout);
    topLayout->setMargin(0);
    topLayout->setSpacing(4);
    topLayout->addWidget(m_countsLabel);
    topLayout->addWidget(m_downloadLabel);
    topLayout->addWidget(m_xapianProgress);
}

void StatusWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()),
            this, SLOT(updateStatus()));
    connect(m_backend, SIGNAL(xapianUpdateStarted()),
            this, SLOT(showXapianProgress()));
    connect(m_backend, SIGNAL(xapianUpdateProgress(int)),
            this, SLOT(updateXapianProgress(int)));
    updateStatus();
}

void StatusWidget::updateStatus()
{
    if (m_backend->xapianIndexNeedsUpdate())
        m_backend->updateXapianIndex();

    int upgradeable = m_backend->packageCount(QApt::Package::Upgradeable);
    bool showChanges = m_backend->areChangesMarked();

    QString availableText = i18ncp("@info:status", "1 package available, ", "%1 packages available, ", m_backend->packageCount());
    QString installText = i18nc("@info:status", "%1 installed, ", m_backend->installedCount());
    QString upgradeableText;

    if (upgradeable > 0 && showChanges) {
        upgradeableText = i18nc("@info:status", "%1 upgradeable,", upgradeable);
    } else {
        upgradeableText = i18nc("@info:status", "%1 upgradeable", upgradeable);
        m_countsLabel->setText(availableText % installText % upgradeableText);
    }

    if (showChanges) {
        int toInstallOrUpgrade = m_backend->toInstallCount();
        int toRemove = m_backend->toRemoveCount();

        QString toInstallOrUpgradeText;
        QString toRemoveText;

        if (toInstallOrUpgrade > 0) {
            toInstallOrUpgradeText = i18nc("@info:status Part of the status label", " %1 to install/upgrade", toInstallOrUpgrade);
        }

        if (toRemove > 0 && toInstallOrUpgrade > 0) {
            toRemoveText = i18nc("@info:status Label for the number of packages pending removal when packages are also pending upgrade",
                                 ", %1 to remove", toRemove);
        } else if (toRemove > 0) {
            toRemoveText = i18nc("@info:status Label for the number of packages pending removal when there are only removals",
                                 " %1 to remove", toRemove);
        }

        m_countsLabel->setText(availableText % installText % upgradeableText %
                               toInstallOrUpgradeText % toRemoveText);

        qint64 installSize = m_backend->installSize();
        if (installSize < 0) {
            installSize = -installSize;
            m_downloadLabel->setText(i18nc("@label showing download and install size", "%1 to download, %2 of space to be freed",
                                           KGlobal::locale()->formatByteSize(m_backend->downloadSize()),
                                           KGlobal::locale()->formatByteSize(installSize)));
        } else {
            m_downloadLabel->setText(i18nc("@label showing download and install size", "%1 to download, %2 of space to be used",
                                           KGlobal::locale()->formatByteSize(m_backend->downloadSize()),
                                           KGlobal::locale()->formatByteSize(installSize)));
        }

        m_downloadLabel->show();
    } else {
        m_downloadLabel->hide();
    }
}

void StatusWidget::showXapianProgress()
{
    m_xapianProgress->show();
    m_xapianTimeout->start();
}

void StatusWidget::hideXapianProgress()
{
    m_xapianProgress->hide();
    m_xapianTimeout->stop();
}

void StatusWidget::updateXapianProgress(int percentage)
{
    m_xapianProgress->setValue(percentage);
    m_xapianTimeout->start();
}
