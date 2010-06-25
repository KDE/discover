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
#include <QtCore/QTextStream>
#include <QtGui/QLabel>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QVBoxLayout>

// KDE
#include <KAction>
#include <KDebug>
#include <KDialog>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KMenu>
#include <KPushButton>
#include <KVBox>
#include <KTemporaryFile>
#include <KTextBrowser>
#include <KTreeWidgetSearchLineWidget>

#include <libqapt/package.h>

#include "ui_MainTab.h"

DetailsWidget::DetailsWidget(QWidget *parent)
    : KTabWidget(parent)
    , m_package(0)
    , m_screenshotFile(0)
    , m_changelogFile(0)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Main tab is in the Ui file. If anybody wants to write a C++ widget that
    // is equivalent to it, I would gladly use it. Layouting sucks with C++
    QWidget *mainTab = new QWidget(this);
    m_mainTab = new Ui::MainTab;
    m_mainTab->setupUi(mainTab);

    m_screenshotButton = new KPushButton(mainTab);
    m_screenshotButton->setIcon(KIcon("image-x-generic"));
    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    connect (m_screenshotButton, SIGNAL(clicked()), this, SLOT(fetchScreenshot()));
    m_mainTab->topHBoxLayout->addWidget(m_screenshotButton);

    m_purgeMenu = new KMenu(m_mainTab->removeButton);
    m_purgeAction = new KAction(this);
    m_purgeMenu->addAction(m_purgeAction);
    m_mainTab->removeButton->setMenu(m_purgeMenu);

    // Technical tab
    m_technicalTab = new QWidget;

    // Dependencies tab
    m_dependenciesTab = new QWidget;

    // File list tab
    m_filesTab = new KVBox;
    m_filesSearchEdit = new KTreeWidgetSearchLineWidget(m_filesTab);
    m_filesTreeWidget = new QTreeWidget(m_filesTab);
    m_filesTreeWidget->setHeaderLabel(i18nc("@title:tab", "Installed Files"));
    m_filesSearchEdit->searchLine()->setTreeWidget(m_filesTreeWidget);

    m_changelogTab = new KVBox;
    m_changelogBrowser = new KTextBrowser(m_changelogTab);


    addTab(mainTab, i18nc("@title:tab", "Details"));
    addTab(m_technicalTab, i18nc("@title:tab", "Technical Details"));
    // TODO: Needs serious work in LibQApt
    // addTab(m_dependenciesTab, i18nc("@title:tab", "Dependencies"));
    addTab(m_changelogTab, i18nc("@title:tab", "Changes List"));

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

    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    m_screenshotButton->setEnabled(true);

    setupButtons(oldPackage);
    refreshButtons();

    m_mainTab->descriptionBrowser->setText(package->longDescription());

    if (package->isInstalled()) {
        addTab(m_filesTab, i18nc("@title:tab", "Installed Files"));
        populateFileList();
    } else {
        if (currentIndex() == indexOf(m_filesTab)) {
            setCurrentIndex(0); // Switch to the main tab
        }
        removeTab(indexOf(m_filesTab));
    }

    if (package->isSupported()) {
        m_mainTab->supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                                 "Canonical provides critical updates for %1 until %2",
                                                 package->name(), package->supportedUntil()));
    } else {
       m_mainTab->supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                                "Canonical does not provide updates for %1. Some updates "
                                                "may be provided by the Ubuntu community", package->name()));
    }

    fetchChangelog();
}

void DetailsWidget::clear()
{
    m_package = 0;
    hide();
}

void DetailsWidget::setupButtons(QApt::Package *oldPackage)
{
    if (oldPackage) {
        disconnect(m_mainTab->installButton, SIGNAL(clicked()), oldPackage, SLOT(setInstall()));
        disconnect(m_mainTab->removeButton, SIGNAL(clicked()), oldPackage, SLOT(setRemove()));
        disconnect(m_mainTab->upgradeButton, SIGNAL(clicked()), oldPackage, SLOT(setInstall()));
        disconnect(m_mainTab->reinstallButton, SIGNAL(clicked()), oldPackage, SLOT(setReInstall()));
        disconnect(m_purgeAction, SIGNAL(triggered()), oldPackage, SLOT(setPurge()));
        disconnect(m_mainTab->cancelButton, SIGNAL(clicked()), oldPackage, SLOT(setKeep()));
    }

    m_mainTab->installButton->setIcon(KIcon("download"));
    m_mainTab->installButton->setText(i18nc("@action:button", "Installation"));
    connect(m_mainTab->installButton, SIGNAL(clicked()), m_package, SLOT(setInstall()));

    m_mainTab->removeButton->setIcon(KIcon("edit-delete"));
    m_mainTab->removeButton->setText(i18nc("@action:button", "Removal"));
    connect(m_mainTab->removeButton, SIGNAL(clicked()), m_package, SLOT(setRemove()));

    m_mainTab->upgradeButton->setIcon(KIcon("system-software-update"));
    m_mainTab->upgradeButton->setText(i18nc("@action:button", "Upgrade"));
    connect(m_mainTab->upgradeButton, SIGNAL(clicked()), m_package, SLOT(setInstall()));

    m_mainTab->reinstallButton->setIcon(KIcon("view-refresh"));
    m_mainTab->reinstallButton->setText(i18nc("@action:button", "Reinstallation"));
    connect(m_mainTab->reinstallButton, SIGNAL(clicked()), m_package, SLOT(setReInstall()));

    m_purgeAction->setIcon(KIcon("edit-delete-shred"));
    m_purgeAction->setText(i18nc("@action:button", "Purge"));
    connect(m_purgeAction, SIGNAL(triggered()), m_package, SLOT(setPurge()));

    // TODO: Downgrade

    m_mainTab->cancelButton->setIcon(KIcon("dialog-cancel"));
    m_mainTab->cancelButton->setText(i18nc("@action:button", "Unmark"));
    connect(m_mainTab->cancelButton, SIGNAL(clicked()), m_package, SLOT(setKeep()));
}

void DetailsWidget::refreshButtons()
{
    if (!m_package) {
        return; // Nothing to refresh yet, so return, else we crash
    }
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
        m_mainTab->cancelButton->hide();
    } else {
        m_mainTab->installButton->show();
        m_mainTab->removeButton->hide();
        m_mainTab->upgradeButton->hide();
        m_mainTab->reinstallButton->hide();
        m_mainTab->cancelButton->hide();
    }

    if (state & (QApt::Package::ToInstall | QApt::Package::ToReInstall |
                 QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
                 QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
        m_mainTab->installButton->hide();
        m_mainTab->removeButton->hide();
        m_mainTab->upgradeButton->hide();
        m_mainTab->reinstallButton->hide();
        m_mainTab->cancelButton->show();
    }
}

void DetailsWidget::populateFileList()
{
    m_filesTreeWidget->clear();
    QStringList filesList = m_package->installedFilesList();

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

// TODO: Cache fetched items, and map them to packages

void DetailsWidget::fetchScreenshot()
{
    m_screenshotFile = new KTemporaryFile;
    m_screenshotFile->setPrefix("muon");
    m_screenshotFile->setSuffix(".png");
    m_screenshotFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_package->screenshotUrl(QApt::Screenshot),
                                           m_screenshotFile->fileName(), -1, KIO::Overwrite);
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(screenshotFetched(KJob*)));
}

void DetailsWidget::screenshotFetched(KJob *job)
{
    if (job->error()) {
        m_screenshotButton->setText(i18nc("@info:status", "No Screenshot Available"));
        m_screenshotButton->setEnabled(false);
        return;
    }
    KDialog *dialog = new KDialog(this);

    QLabel *label = new QLabel(dialog);
    label->setPixmap(QPixmap(m_screenshotFile->fileName()));

    dialog->setWindowTitle(i18nc("@title:window", "Screenshot"));
    dialog->setMainWidget(label);
    dialog->setButtons(KDialog::Close);
    dialog->show();
}

void DetailsWidget::fetchChangelog()
{
    m_changelogFile = new KTemporaryFile;
    m_changelogFile->setPrefix("muon");
    m_changelogFile->setSuffix(".txt");
    m_changelogFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_package->changelogUrl(),
                                      m_changelogFile->fileName(), -1,
                                      KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(changelogFetched(KJob*)));
}

void DetailsWidget::changelogFetched(KJob *job)
{
    QFile changelogFile(m_changelogFile->fileName());
    if (job->error() || !changelogFile.open(QFile::ReadOnly)) {
        m_changelogBrowser->setText(i18nc("@info/rich", "<p>The list of changes is not available yet. "
                                          "Please use <a href='%1'>Launchpad</a> instead.</p>",
                                          QString("http://launchpad.net/ubuntu/+source/" + m_package->sourcePackage())));
        return;
    }
    QTextStream stream(&changelogFile);
    m_changelogBrowser->setText(stream.readAll());
}

#include "DetailsWidget.moc"
