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
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>

// KDE includes
#include <KDebug>
#include <KGlobal>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

StatusWidget::StatusWidget(QWidget *parent)
    : KHBox(parent)
    , m_backend(0)
{
    m_countsLabel = new QLabel(this);
    m_countsLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    m_downloadLabel = new QLabel(this);
    m_downloadLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_xapianProgress = new QProgressBar(this);
    m_xapianProgress->setFormat(i18nc("@status", "Rebuilding Search Index"));
    m_xapianProgress->hide();
}

StatusWidget::~StatusWidget()
{
}

void StatusWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(updateStatus()));
    updateStatus();
}

void StatusWidget::updateStatus()
{
    int upgradeable = m_backend->packageCount(QApt::Package::Upgradeable);
    bool showChanges = (m_backend->markedPackages().count() > 0);

    QString availableText = i18np("1 package available, ", "%1 packages available, ", m_backend->packageCount());
    QString installText = i18n("%1 installed, ", m_backend->packageCount(QApt::Package::Installed));
    QString upgradeableText;

    if (upgradeable > 0 && showChanges) {
        upgradeableText = i18n("%1 upgradeable,", upgradeable);
    } else {
        upgradeableText = i18n("%1 upgradeable", upgradeable);
        m_countsLabel->setText(availableText % installText % upgradeableText);
    }

    if (showChanges) {
        int toInstallOrUpgrade = m_backend->packageCount(QApt::Package::ToInstall);
        int toRemove = m_backend->packageCount((QApt::Package::State)(QApt::Package::ToRemove | QApt::Package::ToPurge));

        QString toInstallOrUpgradeText;
        QString toRemoveText;

        if (toInstallOrUpgrade > 0) {
            toInstallOrUpgradeText = i18nc("Part of the status label", " %1 to install/upgrade", toInstallOrUpgrade);
        }

        if (toRemove > 0 && toInstallOrUpgrade > 0) {
            toRemoveText = i18nc("Label for the number of packages pending removal when packages are also pending upgrade",
                                 ", %1 to remove", toRemove);
        } else if (toRemove > 0) {
            toRemoveText = i18nc("Label for the number of packages pending removal when there are only removals",
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
}

void StatusWidget::hideXapianProgress()
{
    m_xapianProgress->hide();
}

void StatusWidget::updateXapianProgress(int percentage)
{
    m_xapianProgress->setValue(percentage);
}

#include "StatusWidget.moc"

