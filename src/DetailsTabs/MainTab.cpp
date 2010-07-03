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

#include "MainTab.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

// KDE includes
#include <KAction>
#include <KDebug>
#include <KDialog>
#include <KIcon>
#include <KIO/Job>
#include <KJob>
#include <KMenu>
#include <KMessageBox>
#include <KTemporaryFile>

// LibQApt includes
#include <libqapt/backend.h>
#include <libqapt/package.h>

// Own includes
#include "ui_MainTab.h"

MainTab::MainTab(QWidget *parent)
    : QWidget(parent)
    , m_backend(0)
    , m_package(0)
    , m_screenshotFile(0)
{
    // Main tab is in the Ui file. If anybody wants to write a C++ widget that
    // is equivalent to it, I would gladly use it. Layouting sucks with C++
    m_mainTab = new Ui::MainTab;
    m_mainTab->setupUi(this);

    m_screenshotButton = new QPushButton(this);
    m_screenshotButton->setIcon(KIcon("image-x-generic"));
    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    connect(m_screenshotButton, SIGNAL(clicked()), this, SLOT(fetchScreenshot()));
    m_mainTab->topHBoxLayout->addWidget(m_screenshotButton);

    m_purgeMenu = new KMenu(m_mainTab->removeButton);
    m_purgeAction = new KAction(this);
    m_purgeMenu->addAction(m_purgeAction);
    m_mainTab->removeButton->setMenu(m_purgeMenu);
}

MainTab::~MainTab()
{
}

void MainTab::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void MainTab::setPackage(QApt::Package *package)
{
    QApt::Package *oldPackage = m_package;
    qDebug() << "new package set";
    m_package = package;
    m_mainTab->packageShortDescLabel->setText(package->shortDescription());

    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    m_screenshotButton->setEnabled(true);

    setupButtons(oldPackage);
    refreshButtons();

    m_mainTab->descriptionBrowser->setText(package->longDescription());

    if (package->isSupported()) {
        m_mainTab->supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                                 "Canonical provides critical updates for %1 until %2",
                                                 package->name(), package->supportedUntil()));
    } else {
       m_mainTab->supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                                "Canonical does not provide updates for %1. Some updates "
                                                "may be provided by the Ubuntu community", package->name()));
    }
}

void MainTab::clear()
{
    m_package = 0;
}

void MainTab::setupButtons(QApt::Package *oldpackage)
{
    if (oldpackage) {
        disconnect(m_mainTab->installButton, SIGNAL(clicked()), this, SLOT(setInstall()));
        disconnect(m_mainTab->removeButton, SIGNAL(clicked()), this, SLOT(setRemove()));
        disconnect(m_mainTab->upgradeButton, SIGNAL(clicked()), this, SLOT(setInstall()));
        disconnect(m_mainTab->reinstallButton, SIGNAL(clicked()), this, SLOT(setReInstall()));
        disconnect(m_purgeAction, SIGNAL(triggered()), this, SLOT(setPurge()));
        disconnect(m_mainTab->cancelButton, SIGNAL(clicked()), this, SLOT(setKeep()));
    }

    m_mainTab->installButton->setIcon(KIcon("download"));
    m_mainTab->installButton->setText(i18nc("@action:button", "Installation"));
    connect(m_mainTab->installButton, SIGNAL(clicked()), this, SLOT(setInstall()));

    m_mainTab->removeButton->setIcon(KIcon("edit-delete"));
    m_mainTab->removeButton->setText(i18nc("@action:button", "Removal"));
    connect(m_mainTab->removeButton, SIGNAL(clicked()), this, SLOT(setRemove()));

    m_mainTab->upgradeButton->setIcon(KIcon("system-software-update"));
    m_mainTab->upgradeButton->setText(i18nc("@action:button", "Upgrade"));
    connect(m_mainTab->upgradeButton, SIGNAL(clicked()), this, SLOT(setInstall()));

    m_mainTab->reinstallButton->setIcon(KIcon("view-refresh"));
    m_mainTab->reinstallButton->setText(i18nc("@action:button", "Reinstallation"));
    connect(m_mainTab->reinstallButton, SIGNAL(clicked()), this, SLOT(setReInstall()));

    m_purgeAction->setIcon(KIcon("edit-delete-shred"));
    m_purgeAction->setText(i18nc("@action:button", "Purge"));
    connect(m_purgeAction, SIGNAL(triggered()), this, SLOT(setPurge()));

    // TODO: Downgrade

    m_mainTab->cancelButton->setIcon(KIcon("dialog-cancel"));
    m_mainTab->cancelButton->setText(i18nc("@action:button", "Unmark"));
    connect(m_mainTab->cancelButton, SIGNAL(clicked()), this, SLOT(setKeep()));
}

void MainTab::refreshButtons()
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

// TODO: Cache fetched items, and map them to packages

void MainTab::fetchScreenshot()
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

void MainTab::screenshotFetched(KJob *job)
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

bool MainTab::confirmEssentialRemoval()
{
    QString text = i18nc("@label", "Removing this package may break your system. Are you sure you want to remove it?");
    QString title = i18nc("@label", "Warning - Removing Important Package");
    int result = KMessageBox::Cancel;

    result = KMessageBox::warningContinueCancel(this, text, title, KStandardGuiItem::cont(),
                                                KStandardGuiItem::cancel(), QString(), KMessageBox::Dangerous);

    switch (result) {
        case KMessageBox::Continue:
            return true;
            break;
        case KMessageBox::Cancel:
        default:
            return false;
            break;
    }
}

void MainTab::setInstall()
{
    m_oldCacheState = m_backend->currentCacheState();
    if (!m_package->availableVersion().isEmpty()) {
        m_package->setInstall();
    }

    if (m_package->wouldBreak()) {
        showBrokenReason();
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::setRemove()
{
    bool remove = true;
    if (m_package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        m_oldCacheState = m_backend->currentCacheState();
        m_package->setRemove();

        if (m_package->wouldBreak()) {
            showBrokenReason();
            m_backend->restoreCacheState(m_oldCacheState);
        }
    }
}

void MainTab::setUpgrade()
{
    m_oldCacheState = m_backend->currentCacheState();
    m_package->setInstall();

    if (m_package->wouldBreak()) {
        showBrokenReason();
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::setReInstall()
{
    m_oldCacheState = m_backend->currentCacheState();
    m_package->setReInstall();

    if (m_package->wouldBreak()) {
        showBrokenReason();
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::setPurge()
{
    bool remove = true;
    if (m_package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        m_oldCacheState = m_backend->currentCacheState();
        m_package->setPurge();

        if (m_package->wouldBreak()) {
            showBrokenReason();
            m_backend->restoreCacheState(m_oldCacheState);
        }
    }
}

void MainTab::setKeep()
{
    m_oldCacheState = m_backend->currentCacheState();
    m_package->setKeep();

    if (m_package->wouldBreak()) {
        showBrokenReason();
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::showBrokenReason()
{
    kDebug() << "Broken because you're a noob. But I'm not going to let you do that";
}

#include "MainTab.moc"
