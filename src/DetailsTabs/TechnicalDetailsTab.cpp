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

// Own includes
#include "MuonStrings.h"

TechnicalDetailsTab::TechnicalDetailsTab(QWidget *parent)
    : QScrollArea(parent)
    , m_package(0)
{
    m_strings = new MuonStrings(this);
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    viewport()->setAutoFillBackground(false);

    KVBox *mainWidget = new KVBox(this);

    QWidget *generalWidget = new QWidget(mainWidget);
    QGridLayout *generalGrid = new QGridLayout(generalWidget);
    generalWidget->setLayout(generalGrid);

    // generalGrid, row 0
    QLabel *maintainerLabel = new QLabel(generalWidget);
    maintainerLabel->setText(i18nc("@label Label preceding the package maintainer", "Maintainer:"));
    m_maintainer = new QLabel(generalWidget);
    generalGrid->addWidget(maintainerLabel, 0, 0, Qt::AlignRight);
    generalGrid->addWidget(m_maintainer, 0, 1, Qt::AlignLeft);

    // generalGrid, row 1
    QLabel *sectionLabel = new QLabel(generalWidget);
    sectionLabel->setText(i18nc("@label Label preceding the package category", "Category:"));
    m_section = new QLabel(generalWidget);
    generalGrid->addWidget(sectionLabel, 1, 0, Qt::AlignRight);
    generalGrid->addWidget(m_section, 1, 1, Qt::AlignLeft);

    // generalGrid, row 2
    QLabel *sourcePackageLabel = new QLabel(generalWidget);
    sourcePackageLabel->setText(i18nc("@label The parent package that this package comes from",
                                      "Source Package:"));
    m_sourcePackage = new QLabel(generalWidget);
    generalGrid->addWidget(sourcePackageLabel, 2, 0, Qt::AlignRight);
    generalGrid->addWidget(m_sourcePackage, 2, 1, Qt::AlignLeft);

    generalGrid->setColumnStretch(1, 1);

    KHBox *versionWidget = new KHBox(mainWidget);

    m_installedVersionBox = new QGroupBox(versionWidget);
    m_installedVersionBox->setTitle(i18nc("@title:group", "Installed Version"));
    QGridLayout *installedGridLayout = new QGridLayout(m_installedVersionBox);
    m_installedVersionBox->setLayout(installedGridLayout);

    // installedVersionBox, row 0
    QLabel *installedVersionLabel = new QLabel(m_installedVersionBox);
    installedVersionLabel->setText(i18nc("@label Label preceding the package version", "Version:"));
    m_installedVersion = new QLabel(m_installedVersionBox);
    installedGridLayout->addWidget(installedVersionLabel, 0, 0, Qt::AlignRight);
    installedGridLayout->addWidget(m_installedVersion, 0, 1, Qt::AlignLeft);
    // installedVersionBox, row 1
    QLabel *installedSizeLabel = new QLabel(m_installedVersionBox);
    installedSizeLabel->setText(i18nc("@label Label preceding the package size", "Installed Size:"));
    m_installedSize = new QLabel(m_installedVersionBox);
    installedGridLayout->addWidget(installedSizeLabel, 1, 0, Qt::AlignRight);
    installedGridLayout->addWidget(m_installedSize, 1, 1, Qt::AlignLeft);

    installedGridLayout->setRowStretch(3, 1);
    installedGridLayout->setColumnStretch(1, 1);


    m_currentVersionBox = new QGroupBox(versionWidget);
    m_currentVersionBox->setTitle(i18nc("@title:group", "Available Version"));
    QGridLayout *currentGridLayout = new QGridLayout(m_currentVersionBox);
    m_currentVersionBox->setLayout(currentGridLayout);

    // currentVersionBox, row 0
    QLabel *currentVersionLabel = new QLabel(m_currentVersionBox);
    currentVersionLabel->setText(i18nc("@label Label preceeding the package version", "Version:"));
    m_currentVersion = new QLabel(m_currentVersionBox);
    currentGridLayout->addWidget(currentVersionLabel, 0, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_currentVersion, 0, 1, Qt::AlignLeft);
    // currentVersionBox, row 1
    QLabel *currentSizeLabel = new QLabel(m_currentVersionBox);
    currentSizeLabel->setText(i18nc("@label Label preceeding the package size", "Installed Size:"));
    m_currentSize = new QLabel(m_currentVersionBox);
    currentGridLayout->addWidget(currentSizeLabel, 1, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_currentSize, 1, 1, Qt::AlignLeft);
    // currentVersionBox, row 2
    QLabel *downloadSizeLabel = new QLabel(m_currentVersionBox);
    downloadSizeLabel->setText(i18nc("@label Label preceeding the package's download size", "Download Size:"));
    m_downloadSize = new QLabel(m_currentVersionBox);
    currentGridLayout->addWidget(downloadSizeLabel, 2, 0, Qt::AlignRight);
    currentGridLayout->addWidget(m_downloadSize, 2, 1, Qt::AlignLeft);

    currentGridLayout->setColumnStretch(1, 1);

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
    refresh();
}

void TechnicalDetailsTab::refresh()
{
    if (!m_package) {
        return; // Nothing to refresh yet, so return, else we crash
    }

    m_maintainer->setText(m_package->maintainer());
    m_section->setText(m_strings->groupName(m_package->section()));
    m_sourcePackage->setText(m_package->sourcePackage());

    if (m_package->isInstalled()) {
        m_installedVersionBox->show();
        m_installedVersion->setText(m_package->installedVersion());
        m_installedSize->setText(KGlobal::locale()->formatByteSize(m_package->currentInstalledSize()));
    } else {
        m_installedVersionBox->hide();
    }

    m_currentVersion->setText(m_package->availableVersion());
    m_currentSize->setText(KGlobal::locale()->formatByteSize(m_package->availableInstalledSize()));
    m_downloadSize->setText(KGlobal::locale()->formatByteSize(m_package->downloadSize()));
}

#include "TechnicalDetailsTab.moc"
