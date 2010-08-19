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
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KAction>
#include <KDebug>
#include <KDialog>
#include <KHBox>
#include <KIcon>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceWidget>
#include <KTemporaryFile>
#include <KTextBrowser>

// LibQApt includes
#include <libqapt/backend.h>
#include <libqapt/package.h>

MainTab::MainTab(QWidget *parent)
    : QWidget(parent)
    , m_backend(0)
    , m_package(0)
    , m_screenshotFile(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    KHBox *headerBox = new KHBox(this);
    layout->addWidget(headerBox);
    m_packageShortDescLabel = new QLabel(headerBox);
    m_packageShortDescLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont font;
    font.setBold(true);
    m_packageShortDescLabel->setFont(font);

    //Busy widget
    m_throbberWidget = new KPixmapSequenceWidget(headerBox);
    m_throbberWidget->setSequence(KPixmapSequence("process-working", 22));
    m_throbberWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_throbberWidget->hide();
    m_screenshotButton = new QPushButton(headerBox);
    m_screenshotButton->setIcon(KIcon("image-x-generic"));
    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    connect(m_screenshotButton, SIGNAL(clicked()), this, SLOT(fetchScreenshot()));

    KHBox *buttonBox = new KHBox(this);
    layout->addWidget(buttonBox);
    QLabel *buttonLabel = new QLabel(buttonBox);
    buttonLabel->setText(i18nc("@label", "Mark for:"));

    m_installButton = new QPushButton(buttonBox);
    m_installButton->setIcon(KIcon("download"));
    m_installButton->setText(i18nc("@action:button", "Installation"));
    connect(m_installButton, SIGNAL(clicked()), this, SLOT(setInstall()));

    m_removeButton = new QToolButton(buttonBox);
    m_removeButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_removeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_removeButton->setIcon(KIcon("edit-delete"));
    m_removeButton->setText(i18nc("@action:button", "Removal"));
    connect(m_removeButton, SIGNAL(clicked()), this, SLOT(setRemove()));

    m_upgradeButton = new QPushButton(buttonBox);
    m_upgradeButton->setIcon(KIcon("system-software-update"));
    m_upgradeButton->setText(i18nc("@action:button", "Upgrade"));
    connect(m_upgradeButton, SIGNAL(clicked()), this, SLOT(setInstall()));

    m_reinstallButton = new QPushButton(buttonBox);
    m_reinstallButton->setIcon(KIcon("view-refresh"));
    m_reinstallButton->setText(i18nc("@action:button", "Reinstallation"));
    connect(m_reinstallButton, SIGNAL(clicked()), this, SLOT(setReInstall()));

    m_purgeMenu = new KMenu(m_removeButton);
    m_purgeAction = new KAction(this);
    m_purgeAction->setIcon(KIcon("edit-delete-shred"));
    m_purgeAction->setText(i18nc("@action:button", "Purge"));
    connect(m_purgeAction, SIGNAL(triggered()), this, SLOT(setPurge()));
    m_purgeMenu->addAction(m_purgeAction);
    m_removeButton->setMenu(m_purgeMenu);

    m_cancelButton = new QPushButton(buttonBox);
    m_cancelButton->setIcon(KIcon("dialog-cancel"));
    m_cancelButton->setText(i18nc("@action:button", "Unmark"));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(setKeep()));

    QWidget *buttonSpacer = new QWidget(buttonBox);
    buttonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_descriptionBrowser = new KTextBrowser(this);
    layout->addWidget(m_descriptionBrowser);

    m_supportedLabel = new QLabel(this);
    layout->addWidget(m_supportedLabel);
}

MainTab::~MainTab()
{
    delete m_screenshotFile;
}

void MainTab::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void MainTab::setPackage(QApt::Package *package)
{
    m_package = package;
    refresh();
}

void MainTab::clear()
{
    m_package = 0;
}

void MainTab::refresh()
{
    if (!m_package) {
        return; // Nothing to refresh yet, so return, else we crash
    }
    int state = m_package->state();
    bool upgradeable = (state & QApt::Package::Upgradeable);

    if (state & QApt::Package::Installed) {
        m_installButton->hide();
        m_removeButton->show();
        if (upgradeable) {
            m_upgradeButton->show();
        } else {
            m_upgradeButton->hide();
        }
        m_reinstallButton->show();
        m_cancelButton->hide();
    } else {
        m_installButton->show();
        m_removeButton->hide();
        m_upgradeButton->hide();
        m_reinstallButton->hide();
        m_cancelButton->hide();
    }

    if (state & (QApt::Package::ToInstall | QApt::Package::ToReInstall |
                 QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
                 QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
        m_installButton->hide();
        m_removeButton->hide();
        m_upgradeButton->hide();
        m_reinstallButton->hide();
        m_cancelButton->show();
    }

    m_packageShortDescLabel->setText(m_package->shortDescription());

    m_screenshotButton->setText(i18nc("@action:button", "Get Screenshot..."));
    m_screenshotButton->setEnabled(true);

    m_descriptionBrowser->setText(m_package->longDescription());

    if (m_package->isSupported()) {
        m_supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                        "Canonical provides critical updates for %1 until %2",
                                        m_package->name(), m_package->supportedUntil()));
    } else {
        m_supportedLabel->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                        "Canonical does not provide updates for %1. Some updates "
                                        "may be provided by the Ubuntu community", m_package->name()));
    }
}

// TODO: Cache fetched items, and map them to packages

void MainTab::fetchScreenshot()
{
    m_screenshotButton->setEnabled(false);
    m_throbberWidget->show();
    if (m_screenshotFile) {
        m_screenshotFile->deleteLater();
        m_screenshotFile = 0;
    }
    m_screenshotFile = new KTemporaryFile;
    m_screenshotFile->setPrefix("muon");
    m_screenshotFile->setSuffix(".png");
    m_screenshotFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_package->screenshotUrl(QApt::Screenshot),
                               m_screenshotFile->fileName(), -1, KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(screenshotFetched(KJob *)));
}

void MainTab::screenshotFetched(KJob *job)
{
    if (job->error()) {
        m_screenshotButton->setText(i18nc("@info:status", "No Screenshot Available"));
        m_throbberWidget->hide();
        return;
    }
    m_screenshotButton->setEnabled(true);
    KDialog *dialog = new KDialog(this);

    QLabel *label = new QLabel(dialog);
    label->setPixmap(QPixmap(m_screenshotFile->fileName()));

    dialog->setWindowTitle(i18nc("@title:window", "Screenshot"));
    dialog->setMainWidget(label);
    dialog->setButtons(KDialog::Close);
    dialog->show();
    m_throbberWidget->hide();
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
    m_backend->saveCacheState();
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
        m_backend->saveCacheState();
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
    m_backend->saveCacheState();
    m_package->setInstall();

    if (m_package->wouldBreak()) {
        showBrokenReason();
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::setReInstall()
{
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
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
        m_backend->saveCacheState();
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
    m_backend->saveCacheState();
    m_package->setKeep();

    if (m_package->wouldBreak()) {
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void MainTab::showBrokenReason()
{
    QHash<int, QHash<QString, QVariantMap> > failedReasons = m_package->brokenReason();
    QString reason;
    QString dialogText = i18nc("@label", "The \"%1\" package could not be marked for installation or upgrade:",
                               m_package->name());
    dialogText += '\n';
    QString title = i18nc("@title:window", "Unable to Mark Package");

    QHash<int, QHash<QString, QVariantMap> >::const_iterator reasonIter = failedReasons.constBegin();
    QHash<int, QHash<QString, QVariantMap> >::const_iterator end = failedReasons.constEnd();
    while (reasonIter != end) {
        QApt::BrokenReason failType = (QApt::BrokenReason)reasonIter.key();
        QHash<QString, QVariantMap> failReason = reasonIter.value();
        dialogText += digestReason(failType, failReason);

        reasonIter++;
    }

    KMessageBox::information(this, dialogText, title);
}

QString MainTab::digestReason(QApt::BrokenReason failType, QHash<QString, QVariantMap> failReason)
{
    QHash<QString, QVariantMap>::const_iterator packageIter = failReason.constBegin();
    QHash<QString, QVariantMap>::const_iterator end = failReason.constEnd();
    QString reason;

    switch (failType) {
    case QApt::ParentNotInstallable: {
        reason += '\t';
        reason = i18nc("@label", "The \"%1\" package has no available version, but exists in the database.\n"
                       "\tThis typically means that the package was mentioned in a dependency and "
                       "never uploaded, has been obsoleted, or is not available from the currently-enabled "
                       "repositories.", m_package->name());
        break;
    }
    case QApt::WrongCandidateVersion: {
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            QString requiredVersion = packageIter.value()["RequiredVersion"].toString();
            QString candidateVersion = packageIter.value()["CandidateVersion"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt 0.1, but 0.2 is to be installed",
                                "%1: %2 %3, but %4 is to be installed",
                                relation, package, requiredVersion, candidateVersion);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt 0.1, but 0.2 is to be installed",
                                        "or %1 %2, but %3 is to be installed",
                                        package, requiredVersion, candidateVersion));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    }
    case QApt::DepNotInstallable: {
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt, but is not installable",
                                "%1: %2, but it is not installable",
                                relation, package);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt, but is not installable",
                                        "or %1, but is not installable",
                                        package));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    }
    case QApt::VirtualPackage:
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt, but it is a virtual package",
                                "%1: %2, but it is a virtual package",
                                relation, package);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt, but it is a virtual package",
                                        "or %1, but it is a virtual package",
                                        package));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    default:
        break;
    }

    return reason;
}

#include "MainTab.moc"
