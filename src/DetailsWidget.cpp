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
#include <KAction>
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

    setupPackageActions();
    m_actionMenu = new KMenu(mainTab);
    m_mainTab->actionToolButton->setMenu(m_actionMenu);
    connect(m_mainTab->actionToolButton, SIGNAL(triggered(QAction *)),
            this, SLOT(markPackage(QAction *)));

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
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::setPackage(QApt::Package *package)
{
    kDebug() << "Setting package";
    m_package = package;
    m_mainTab->packageShortDescLabel->setText(package->shortDescription());

    m_screenshotButton->show();
    m_mainTab->actionToolButton->setEnabled(true);
    m_mainTab->actionToolButton->setText(i18n("Choose an action"));
    m_actionMenu->clear();

    m_mainTab->changelogBrowser->setText(package->longDescription());

    if (package->isInstalled()) {
        addTab(m_filesTab, i18n("Installed Files"));
        populateFileList(package);

        //TODO: split into updateActionMenu(bool installed)
        //TODO: Handle coming back to the package too, in case we marked something
        // and are coming back
        if (package->state() & QApt::Package::Upgradeable) {
            m_actionMenu->addAction(m_upgradeAction);
        }

        m_actionMenu->addAction(m_removeAction);
        m_actionMenu->addAction(m_reinstallAction);
        m_actionMenu->addAction(m_purgeAction);
    } else {
        setCurrentIndex(0); // Switch to the main tab
        removeTab(indexOf(m_filesTab));

        m_actionMenu->addAction(m_installAction);
    }
}

void DetailsWidget::setupPackageActions()
{
    m_installAction = new KAction(this);
    m_installAction->setIcon(KIcon("download"));
    m_installAction->setText(i18n("Install"));

    m_removeAction = new KAction(this);
    m_removeAction->setIcon(KIcon("edit-delete"));
    m_removeAction->setText(i18n("Remove"));

    m_upgradeAction = new KAction(this);
    m_upgradeAction->setIcon(KIcon("system-software-update"));
    m_upgradeAction->setText(i18n("Upgrade"));

    m_reinstallAction = new KAction(this);
    m_reinstallAction->setIcon(KIcon("view-refresh"));
    m_reinstallAction->setText(i18n("Reinstall"));

    m_purgeAction = new KAction(this);
    m_purgeAction->setIcon(KIcon("edit-delete-shred"));
    m_purgeAction->setText(i18n("Purge"));

//     m_downgradeAction = new KAction(this);
//     m_downgradeAction->setText(i18n("Downgrade"));

    m_cancelAction = new KAction(this);
    m_cancelAction->setIcon(KIcon("dialog-cancel"));
    m_cancelAction->setText(i18n("Cancel"));
}

void DetailsWidget::markPackage(QAction *action)
{
    if (action == m_cancelAction) {
        m_package->setKeep();
        // TODO: FAIL. Split out menu setting code into own function so that
        // we don't have to reload the whole thing by calling setPacage();
        setPackage(m_package);
        m_mainTab->actionToolButton->setIcon(KIcon());
        m_mainTab->actionToolButton->setText(i18n("Choose an action"));
    } else {
        if (action == m_installAction) {
            m_package->setInstall();
            m_mainTab->actionToolButton->setIcon(KIcon("download"));
            m_mainTab->actionToolButton->setText(i18n("Install"));
        } else if (action == m_removeAction) {
            m_package->setRemove();
            m_mainTab->actionToolButton->setIcon(KIcon("edit-delete"));
            m_mainTab->actionToolButton->setText(i18n("Remove"));
        } else if (action == m_upgradeAction) {
          m_package->setInstall(); // Upgrade == install for apt
          m_mainTab->actionToolButton->setIcon(KIcon("view-refresh"));
          m_mainTab->actionToolButton->setText(i18n("Upgrade"));
        } else if (action == m_reinstallAction) {
          m_package->setReInstall();
          m_mainTab->actionToolButton->setText(i18n("Reinstall"));
        } else if (action == m_purgeAction) {
          m_package->setRemove(/*bool purge*/ true);
          m_mainTab->actionToolButton->setIcon(KIcon("edit-delete-shred"));
          m_mainTab->actionToolButton->setText(i18n("Purge"));
        }
        m_actionMenu->clear();
        m_actionMenu->addAction(m_cancelAction);
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
