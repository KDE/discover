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

#include "TechnicalDetailsTab.h"

// Qt includes
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>

// KDE includes
#include <KGlobal>
#include <KLocale>
#include <KVBox>
#include <KHBox>

// LibQApt includes
#include <libqapt/package.h>

TechnicalDetailsTab::TechnicalDetailsTab(QWidget *parent)
    : QScrollArea(parent)
    , m_package(0)
{
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    viewport()->setAutoFillBackground(false);

    KVBox *mainWidget = new KVBox(this);
    KHBox *versionWidget = new KHBox(mainWidget);

    QGroupBox *installedVersionBox = new QGroupBox(versionWidget);
    installedVersionBox->setTitle(i18nc("@title:group", "Installed Version"));
    QGridLayout *installedGridLayout = new QGridLayout(installedVersionBox);
    installedVersionBox->setLayout(installedGridLayout);

    // installedVersionBox, row 0
    QLabel *installedVersionLabel = new QLabel(installedVersionBox);
    installedVersionLabel->setText(i18nc("@label Label preceeding the package version", "Version:"));
    m_installedVersion = new QLabel(installedVersionBox);
    installedGridLayout->addWidget(installedVersionLabel, 0, 0, Qt::AlignRight);
    installedGridLayout->addWidget(m_installedVersion, 0, 1, Qt::AlignLeft);
    // installedVersionBox, row 1
    QLabel *installedSizeLabel = new QLabel(installedVersionBox);
    installedSizeLabel->setText(i18nc("@label Label preceeding the package size", "Installed Size:"));
    m_installedSize = new QLabel(installedVersionBox);
    installedGridLayout->addWidget(installedSizeLabel, 1, 0, Qt::AlignRight);
    installedGridLayout->addWidget(m_installedSize, 1, 1, Qt::AlignLeft);
    //TODO: Spacer for the 3rd row


    QGroupBox *currentVersionBox = new QGroupBox(versionWidget);
    currentVersionBox->setTitle(i18nc("@title:group", "Available Version"));
    QGridLayout *currentGridLayout = new QGridLayout(currentVersionBox);
    currentVersionBox->setLayout(currentGridLayout);

    // currentVersionBox, row 0
    QLabel *currentVersionLabel = new QLabel(currentVersionBox);
    currentVersionLabel->setText(i18nc("@label Label preceeding the package version", "Version:"));
    m_currentVersion = new QLabel(currentVersionBox);
    currentGridLayout->addWidget(currentVersionLabel, 0, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_currentVersion, 0, 1, Qt::AlignLeft);
    // currentVersionBox, row 1
    QLabel *currentSizeLabel = new QLabel(currentVersionBox);
    currentSizeLabel->setText(i18nc("@label Label preceeding the package size", "Installed Size:"));
    m_currentSize = new QLabel(currentVersionBox);
    currentGridLayout->addWidget(currentSizeLabel, 1, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_currentSize, 1, 1, Qt::AlignLeft);
    // currentVersionBox, row 2
    QLabel *downloadSizeLabel = new QLabel(currentVersionBox);
    downloadSizeLabel->setText(i18nc("@label Label preceeding the package's download size", "Download Size:"));
    m_downloadSize = new QLabel(currentVersionBox);
    currentGridLayout->addWidget(downloadSizeLabel, 2, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_downloadSize, 2, 1, Qt::AlignLeft);

    QWidget *verticalSpacer = new QWidget(mainWidget);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setWidget(mainWidget);
}

TechnicalDetailsTab::~TechnicalDetailsTab()
{
}

void TechnicalDetailsTab::setPackage(QApt::Package *package)
{
    m_package = package;

    if (package->isInstalled()) {
        m_installedVersion->setText(package->installedVersion());
        m_installedSize->setText(KGlobal::locale()->formatByteSize(package->currentInstalledSize()));
    } else {
        // TODO: Hide groupbox
    }

    m_currentVersion->setText(package->availableVersion());
    m_currentSize->setText(KGlobal::locale()->formatByteSize(package->availableInstalledSize()));
    m_downloadSize->setText(KGlobal::locale()->formatByteSize(package->downloadSize()));
    
}