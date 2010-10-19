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

#include "MuonMainWindow.h"

// Qt includes
#include <QStringBuilder>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KDebug>
#include <KFileDialog>
#include <KMessageBox>

// LibQApt includes
#include <LibQApt/Backend>

MuonMainWindow::MuonMainWindow()
    : KXmlGuiWindow(0)
    , m_backend(0)
    , m_powerInhibitor(0)
    , m_canExit(true)
{
}

MuonMainWindow::~MuonMainWindow()
{
}

void MuonMainWindow::initObject()
{
    m_backend = new QApt::Backend;
    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode, const QVariantMap &)),
            this, SLOT(errorOccurred(QApt::ErrorCode, const QVariantMap &)));
    connect(m_backend, SIGNAL(warningOccurred(QApt::WarningCode, const QVariantMap &)),
            this, SLOT(warningOccurred(QApt::WarningCode, const QVariantMap &)));
    connect(m_backend, SIGNAL(questionOccurred(QApt::WorkerQuestion, const QVariantMap &)),
            this, SLOT(questionOccurred(QApt::WorkerQuestion, const QVariantMap &)));
    m_backend->init();

    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    emit backendReady(m_backend);
}

void MuonMainWindow::slotQuit()
{
    if (queryExit()) {
        KApplication::instance()->quit();
    }
}

bool MuonMainWindow::queryExit()
{
    // We don't want to quit during the middle of a commit
    if (!m_canExit) {
        return false;
    }

    if (m_backend->markedPackages().size() > 0) {
        QString text = i18nc("@label", "There are marked changes that have not yet "
                             "been applied. Do you want to save your changes "
                             "or discard them?");
        int res = KMessageBox::Cancel;
        res = KMessageBox::warningYesNoCancel(this, text, QString(), KStandardGuiItem::saveAs(),
                                              KStandardGuiItem::discard(), KStandardGuiItem::cancel(),
                                              "quitwithoutsave");
        switch (res) {
        case KMessageBox::Yes:
            if (saveSelections()) {
                return true;
            } else {
                // In case of save failure, try again as to not lose data
                queryExit();
            }
            break;
        case KMessageBox::No:
            return true;
        case KMessageBox::Cancel:
            return false;
        }
    }

    return true;
}

void MuonMainWindow::setupActions()
{
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);

    m_updateAction = actionCollection()->addAction("update");
    m_updateAction->setIcon(KIcon("system-software-update"));
    m_updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    m_updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(m_updateAction, SIGNAL(triggered()), this, SLOT(checkForUpdates()));

    m_undoAction = KStandardAction::undo(this, SLOT(undo()), actionCollection());
    actionCollection()->addAction("undo", m_undoAction);

    m_redoAction = KStandardAction::redo(this, SLOT(redo()), actionCollection());
    actionCollection()->addAction("redo", m_redoAction);

    m_revertAction = actionCollection()->addAction("revert");
    m_revertAction->setIcon(KIcon("document-revert"));
    m_revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));
}

void MuonMainWindow::workerEvent(QApt::WorkerEvent event)
{
    Q_UNUSED(event);
}

void MuonMainWindow::errorOccurred(QApt::ErrorCode code, const QVariantMap &args)
{
    QString text;
    QString title;

    switch (code) {
    case QApt::InitError: {
        text = i18nc("@label",
                     "The package system could not be initialized, your "
                     "configuration may be broken.");
        title = i18nc("@title:window", "Initialization error");
        QString details = args["ErrorText"].toString();
        KMessageBox::detailedError(this, text, details, title);
        KApplication::instance()->quit();
        break;
    }
    case QApt::LockError:
        text = i18nc("@label",
                     "Another application seems to be using the package "
                     "system at this time. You must close all other package "
                     "managers before you will be able to install or remove "
                     "any packages.");
        title = i18nc("@title:window", "Unable to obtain package system lock");
        KMessageBox::error(this, text, title);
        break;
    case QApt::DiskSpaceError: {
        QString drive = args["DirectoryString"].toString();
        text = i18nc("@label",
                     "You do not have enough disk space in the directory "
                     "at %1 to continue with this operation.", drive);
        title = i18nc("@title:window", "Low disk space");
        KMessageBox::error(this, text, title);
    }
    case QApt::FetchError:
        text = i18nc("@label",
                     "Could not download packages");
        title = i18nc("@title:window", "Download failed");
        KMessageBox::error(this, text, title);
        break;
    case QApt::CommitError: {
        m_errorStack.append(args);
        reload();
        break;
    }
    case QApt::AuthError:
        text = i18nc("@label",
                     "This operation cannot continue since proper "
                     "authorization was not provided");
        title = i18nc("@title:window", "Authentication error");
        KMessageBox::error(this, text, title);
        break;
    case QApt::WorkerDisappeared:
        text = i18nc("@label", "It appears that the QApt worker has either crashed "
                     "or disappeared. Please report a bug to the QApt maintainers");
        title = i18nc("@title:window", "Unexpected Error");
        KMessageBox::error(this, text, title);
        reload();
        break;
    case QApt::UntrustedError: {
        QStringList untrustedItems = args["UntrustedItems"].toStringList();
        if (untrustedItems.size() == 1) {
            text = i18ncp("@label",
                          "The following package has not been verified by its author. "
                          "Downloading untrusted packages has been disallowed "
                          "by your current configuration.",
                          "The following packages have not been verified by "
                          "their authors. "
                          "Downloading untrusted packages has "
                          "been disallowed by your current configuration.",
                          untrustedItems.size());
        }
        title = i18nc("@title:window", "Untrusted Packages");
        KMessageBox::errorList(this, text, untrustedItems, title);
        break;
    }
    case QApt::UserCancelError:
    case QApt::UnknownError:
    default:
        break;
    }
    m_canExit = true; // If we were committing changes, we aren't anymore
}

void MuonMainWindow::warningOccurred(QApt::WarningCode warning, const QVariantMap &args)
{
    switch (warning) {
    case QApt::SizeMismatchWarning: {
        QString text = i18nc("@label",
                             "The size of the downloaded items did not equal the expected size.");
        QString title = i18nc("@title:window", "Size Mismatch");
        KMessageBox::sorry(this, text, title);
        break;
    }
    case QApt::FetchFailedWarning: {
        m_warningStack.append(args);
        break;
    }
    case QApt::UnknownWarning:
    default:
        break;
    }
}

void MuonMainWindow::questionOccurred(QApt::WorkerQuestion code, const QVariantMap &args)
{
    QVariantMap response;

    switch (code) {
    case QApt::MediaChange: {
        QString media = args["Media"].toString();
        QString drive = args["Drive"].toString();

        QString title = i18nc("@title:window", "Media Change Required");
        QString text = i18nc("@label Asks for a CD change", "Please insert %1 into <filename>%2</filename>", media, drive);

        KMessageBox::information(this, text, title);
        response["MediaChanged"] = true;
        m_backend->answerWorkerQuestion(response);
    }
    case QApt::InstallUntrusted: {
        QStringList untrustedItems = args["UntrustedItems"].toStringList();

        QString title = i18nc("@title:window", "Warning - Unverified Software");
        QString text = i18ncp("@label",
                              "The following piece of software cannot be verified. "
                              "<warning>Installing unverified software represents a "
                              "security risk, as the presence of unverifiable software "
                              "can be a sign of tampering.</warning> Do you wish to continue?",
                              "The following pieces of software cannot be authenticated. "
                              "<warning>Installing unverified software represents a "
                              "security risk, as the presence of unverifiable software "
                              "can be a sign of tampering.</warning> Do you wish to continue?",
                              untrustedItems.size());
        int result = KMessageBox::Cancel;
        bool installUntrusted = false;

        result = KMessageBox::warningContinueCancelList(this, text,
                 untrustedItems, title);
        switch (result) {
        case KMessageBox::Continue:
            installUntrusted = true;
            break;
        case KMessageBox::Cancel:
            installUntrusted = false;
            reload(); //Pseudo-error in this case. Reset things
            break;
        }

        response["InstallUntrusted"] = installUntrusted;
        m_backend->answerWorkerQuestion(response);
    }
    case QApt::InvalidQuestion:
    default:
        break;
    }
}

void MuonMainWindow::showQueuedWarnings()
{
    QString details;
    QString text = i18nc("@label", "Unable to download the following packages:");
    foreach (const QVariantMap &args, m_warningStack) {
        QString failedItem = args["FailedItem"].toString();
        QString warningText = args["WarningText"].toString();
        details.append(i18nc("@label",
                             "Failed to download %1\n"
                             "%2\n\n", failedItem, warningText));
    }
    QString title = i18nc("@title:window", "Some Packages Could not be Downloaded");
    KMessageBox::detailedError(this, text, details, title);
}

void MuonMainWindow::showQueuedErrors()
{
    QString details;
    QString text = i18ncp("@label", "An error occurred while applying changes:",
                                    "The following errors occurred while applying changes:",
                                    m_warningStack.size());
    foreach (const QVariantMap &args, m_errorStack) {
        QString failedItem = i18nc("@label Shows which package failed", "Package: %1", args["FailedItem"].toString());
        QString errorText = i18nc("@label Shows the error", "Error: %1", args["ErrorText"].toString());
        details.append(failedItem % QLatin1Char('\n') % errorText % QLatin1Literal("\n\n"));
    }

    QString title = i18nc("@title:window", "Commit error");
    KMessageBox::detailedError(this, text, details, title);
}

void MuonMainWindow::reload()
{
}

bool MuonMainWindow::saveSelections()
{
    QString filename;

    filename = KFileDialog::getSaveFileName(QString(), QString(), this,
                                            i18nc("@title:window", "Save Markings As"));

    if (filename.isEmpty()) {
        return false;
    }

    if (!m_backend->saveSelections(filename)) {
        QString text = i18nc("@label", "The document could not be saved, as it "
                             "was not possible to write to "
                             "<filename>%1</filename>\n\nCheck "
                             "that you have write access to this file "
                             "or that enough disk space is available.",
                             filename);
        KMessageBox::error(this, text, QString());
        return false;
    }

    return true;
}

bool MuonMainWindow::saveInstalledPackagesList()
{
    QString filename;

    filename = KFileDialog::getSaveFileName(QString(), QString(), this,
                                            i18nc("@title:window", "Save Installed Packages List As"));

    if (filename.isEmpty()) {
        return false;
    }

    if (!m_backend->saveInstalledPackagesList(filename)) {
        QString text = i18nc("@label", "The document could not be saved, as it "
                             "was not possible to write to "
                             "<filename>%1</filename>\n\nCheck "
                             "that you have write access to this file "
                             "or that enough disk space is available.",
                             filename);
        KMessageBox::error(this, text, QString());
        return false;
    }

    return true;
}

void MuonMainWindow::loadSelections()
{
    QString filename;

    filename = KFileDialog::getOpenFileName(QString(), QString(), this, i18nc("@title:window", "Open File"));

    if (filename.isEmpty()) {
        return;
    }

    if (!m_backend->loadSelections(filename)) {
        QString text = i18nc("@label", "Could not mark changes. Please make sure "
                             "that the file is a markings file created by "
                             "either the Muon Package Manager or the "
                             "Synaptic Package Manager.");
        KMessageBox::error(this, text, QString());
    }
}

void MuonMainWindow::undo()
{
    m_backend->undo();
}

void MuonMainWindow::redo()
{
    m_backend->redo();
}

void MuonMainWindow::revertChanges()
{
    m_backend->init();
    reload();
}

#include "MuonMainWindow.moc"
