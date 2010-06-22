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

#include "DetailsWidget.h"

// Qt
#include <QtGui/QLabel>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QVBoxLayout>

// KDE
#include <KDebug>
#include <KLocale>
#include <KMenu>
#include <KPushButton>
#include <KVBox>
#include <KTreeWidgetSearchLineWidget>

#include <libqapt/package.h>

#include "ui_MainTab.h"

DetailsWidget::DetailsWidget(QWidget *parent)
    : KTabWidget(parent)
    , m_package(0)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Main tab is in the Ui file. If anybody wants to write a C++ widget that
    // is equivalent to it, I would gladly use it. Layouting sucks with C++
    QWidget *mainTab = new QWidget(this);
    m_mainTab = new Ui::MainTab;
    m_mainTab->setupUi(mainTab);

    m_screenshotButton = new KPushButton(mainTab);
    m_screenshotButton->setIcon(KIcon("image-x-generic"));
    m_screenshotButton->setText("Get Screenshot");
    m_screenshotButton->hide();
    m_mainTab->topHBoxLayout->addWidget(m_screenshotButton);

    // Technical tab
    m_technicalTab = new QWidget;

    // Dependencies tab
    m_dependenciesTab = new QWidget;

    // File list tab
    m_filesTab = new KVBox;
    m_filesSearchEdit = new KTreeWidgetSearchLineWidget(m_filesTab);
    m_filesTreeWidget = new QTreeWidget(m_filesTab);
    m_filesTreeWidget->setHeaderLabel(i18n("Installed Files"));
    m_filesSearchEdit->searchLine()->setTreeWidget(m_filesTreeWidget);

    // Changelog tab
    m_changelogTab = new QWidget;


    addTab(mainTab, i18n("Details"));
    addTab(m_technicalTab, i18n("Technical Details"));
    addTab(m_dependenciesTab, i18n("Dependencies"));

    // Hide until a package is clicked
    hide();
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::setPackage(QApt::Package *package)
{
    show();
    QApt::Package *oldPackage = m_package;
    m_package = package;
    m_mainTab->packageShortDescLabel->setText(package->shortDescription());

    m_screenshotButton->show();
    setupButtons(oldPackage);
    refreshButtons();

    m_mainTab->changelogBrowser->setText(package->longDescription());

    if (package->isInstalled()) {
        addTab(m_filesTab, i18n("Installed Files"));
        populateFileList(package);
    } else {
        setCurrentIndex(0); // Switch to the main tab
        removeTab(indexOf(m_filesTab));
    }

    if (package->isSupported()) {
        m_mainTab->supportedLabel->setText(i18n("Canonical provides critical updates for %1 until %2",
                                                package->name(), package->supportedUntil()));
    } else {
       m_mainTab->supportedLabel->setText(i18n("Canonical does not provide updates for %1. Some updates "
                                               "may be provided by the Ubuntu community", package->name()));
    }
}

void DetailsWidget::setupButtons(QApt::Package *oldPackage)
{
    if (oldPackage) {
        disconnect(m_mainTab->installButton, SIGNAL(clicked()), oldPackage, SLOT(setInstall()));
        disconnect(m_mainTab->removeButton, SIGNAL(clicked()), oldPackage, SLOT(setRemove()));
        disconnect(m_mainTab->upgradeButton, SIGNAL(clicked()), oldPackage, SLOT(setInstall()));
        disconnect(m_mainTab->reinstallButton, SIGNAL(clicked()), oldPackage, SLOT(setReInstall()));
        disconnect(m_mainTab->purgeButton, SIGNAL(clicked()), oldPackage, SLOT(setPurge()));
        disconnect(m_mainTab->cancelButton, SIGNAL(clicked()), oldPackage, SLOT(setKeep()));
    }

    m_mainTab->installButton->setIcon(KIcon("download"));
    m_mainTab->installButton->setText(i18n("Mark for Installation"));
    connect(m_mainTab->installButton, SIGNAL(clicked()), m_package, SLOT(setInstall()));

    m_mainTab->removeButton->setIcon(KIcon("edit-delete"));
    m_mainTab->removeButton->setText(i18n("Mark for Removal"));
    connect(m_mainTab->removeButton, SIGNAL(clicked()), m_package, SLOT(setRemove()));

    m_mainTab->upgradeButton->setIcon(KIcon("system-software-update"));
    m_mainTab->upgradeButton->setText(i18n("Mark for Upgrade"));
    connect(m_mainTab->upgradeButton, SIGNAL(clicked()), m_package, SLOT(setInstall()));

    m_mainTab->reinstallButton->setIcon(KIcon("view-refresh"));
    m_mainTab->reinstallButton->setText(i18n("Mark for Reinstallation"));
    connect(m_mainTab->reinstallButton, SIGNAL(clicked()), m_package, SLOT(setReInstall()));

    m_mainTab->purgeButton->setIcon(KIcon("edit-delete-shred"));
    m_mainTab->purgeButton->setText(i18n("Mark for Purge"));
    connect(m_mainTab->purgeButton, SIGNAL(clicked()), m_package, SLOT(setPurge()));

    // TODO: Downgrade

    m_mainTab->cancelButton->setIcon(KIcon("dialog-cancel"));
    m_mainTab->cancelButton->setText(i18n("Unmark"));
    connect(m_mainTab->cancelButton, SIGNAL(clicked()), m_package, SLOT(setKeep()));
}

void DetailsWidget::refreshButtons()
{
    int state = m_package->state();
    bool upgradeable = (state & QApt::Package::Upgradeable);

    if (state & QApt::Package::Installed) {
        m_mainTab->installButton->hide();
        m_mainTab->removeButton->show();
        if (upgradeable) {
            m_mainTab->upgradeButton->show();
        } else {
            m_mainTab->upgradeButton->hide();
        }
        m_mainTab->reinstallButton->show();
        m_mainTab->purgeButton->show();
        m_mainTab->cancelButton->hide();
    } else {
        m_mainTab->installButton->show();
        m_mainTab->removeButton->hide();
        m_mainTab->upgradeButton->hide();
        m_mainTab->reinstallButton->hide();
        m_mainTab->purgeButton->hide();
        m_mainTab->cancelButton->hide();
    }

    if (state & (QApt::Package::ToInstall | QApt::Package::ToReInstall |
                 QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
                 QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
        m_mainTab->installButton->hide();
        m_mainTab->removeButton->hide();
        m_mainTab->upgradeButton->hide();
        m_mainTab->reinstallButton->hide();
        m_mainTab->purgeButton->hide();
        m_mainTab->cancelButton->show();
    }
}

void DetailsWidget::populateFileList(QApt::Package *package)
{
    m_filesTreeWidget->clear();
    QStringList filesList = package->installedFilesList();

    foreach (const QString &file, filesList) {
        QStringList split = file.split(QChar('/'));
        QTreeWidgetItem *parentItem = 0;
        foreach (const QString &spl, split) {
            if (spl.isEmpty()) {
                continue;
            }
            if (parentItem) {
                bool there = false;
                int j = parentItem->childCount();
                for (int i = 0; i != j; i++) {
                    if (parentItem->child(i)->text(0) == spl) {
                        there = true;
                        parentItem = parentItem->child(i);
                        break;
                    }
                }
                if (!there) {
                    parentItem = new QTreeWidgetItem(parentItem, QStringList(spl));
                }
            } else {
                QList<QTreeWidgetItem*> list = m_filesTreeWidget->findItems(spl, Qt::MatchExactly);
                if (!list.isEmpty()) {
                    parentItem = list.first();
                } else {
                    parentItem = new QTreeWidgetItem(m_filesTreeWidget, QStringList(spl));
                }
            }
        }
    }
}

#include "DetailsWidget.moc"
