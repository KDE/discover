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
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QStandardItemModel>

// KDE includes
#include <KDebug>
#include <KHBox>
#include <KIcon>
#include <KLocale>

// LibQApt includes
#include <libqapt/package.h>

VersionTab::VersionTab(QWidget *parent)
    : KVBox(parent)
    , m_package(0)
    , m_versions()
{
    KHBox *headerWidget = new KHBox(this);
    QLabel *infoIconLabel = new QLabel(headerWidget);
    infoIconLabel->setPixmap(KIcon("dialog-warning").pixmap(32, 32));
    QLabel *infoLabel = new QLabel(headerWidget);
    infoLabel->setWordWrap(true);
    infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    infoLabel->setText(i18nc("@label", "Muon always selects the most "
                                       "applicable version available. If "
                                       "you force a different version from the "
                                       "default one, errors in the dependency "
                                       "handling can occur."));

    m_versionModel = new QStandardItemModel(this);
    m_versionsView = new QListView(this);
    m_versionsView->setModel(m_versionModel);
    connect(m_versionsView, SIGNAL(activated(const QModelIndex &)), this, SLOT(enableButton()));

    m_forceButton = new QPushButton(this);
    m_forceButton->setText(i18nc("@action:button", "Force Version"));
    m_forceButton->setEnabled(false);
    connect(m_forceButton, SIGNAL(clicked()), this, SLOT(forceVersion()));
}

VersionTab::~VersionTab()
{
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
