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

#include "VersionTab.h"

// Qt includes
#include <QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QListView>
#include <QtWidgets/QPushButton>
#include <QStandardItemModel>

// KDE includes
#include <KDialog>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Package>

VersionTab::VersionTab(QWidget *parent)
    : DetailsTab(parent)
{
    m_name = i18nc("@title:tab", "Versions");

    QLabel *label = new QLabel(this);
    label->setText(i18nc("@label", "Available versions:"));

    m_versionModel = new QStandardItemModel(this);
    m_versionsView = new QListView(this);
    m_versionsView->setModel(m_versionModel);
    connect(m_versionsView, SIGNAL(activated(QModelIndex)), this, SLOT(enableButton()));

    QWidget *footerWidget = new QWidget(this);
    QHBoxLayout *footerLayout = new QHBoxLayout(footerWidget);
    footerWidget->setLayout(footerLayout);

    QLabel *infoIconLabel = new QLabel(footerWidget);
    infoIconLabel->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(32, 32));
    footerLayout->addWidget(infoIconLabel);

    QLabel *infoLabel = new QLabel(footerWidget);
    infoLabel->setWordWrap(true);
    infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    infoLabel->setText(i18nc("@label", "Muon always selects the most "
                             "applicable version available. If "
                             "you force a different version from the "
                             "default one, errors in the dependency "
                             "handling can occur."));
    footerLayout->addWidget(infoLabel);

    m_forceButton = new QPushButton(footerWidget);
    m_forceButton->setText(i18nc("@action:button", "Force Version"));
    m_forceButton->setEnabled(false);
    connect(m_forceButton, SIGNAL(clicked()), this, SLOT(forceVersion()));
    footerLayout->addWidget(m_forceButton);

    m_layout->addWidget(label);
    m_layout->addWidget(m_versionsView);
    m_layout->addWidget(footerWidget);
}

bool VersionTab::shouldShow() const
{
    if (!m_package) {
        return false;
    }

    return m_package->availableVersions().size() > 1;
}

void VersionTab::enableButton()
{
    m_forceButton->setEnabled(true);
}

void VersionTab::setPackage(QApt::Package *package)
{
    m_package = package;
    m_versionModel->clear();
    populateVersions();
}

void VersionTab::populateVersions()
{
    QStringList availableVersions = m_package->availableVersions();

    foreach(const QString &version, availableVersions) {
        QStringList split = version.split(' ');
        m_versions.append(split.at(0));

        QStandardItem *versionItem = new QStandardItem;
        versionItem->setEditable(false);
        versionItem->setText(version);
        m_versionModel->appendRow(versionItem);
    }
}

void VersionTab::forceVersion()
{
    m_package->setVersion(m_versions.at(m_versionsView->currentIndex().row()));
    m_package->setInstall();
}

#include "VersionTab.moc"
