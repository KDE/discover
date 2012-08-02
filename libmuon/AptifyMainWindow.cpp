/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AptifyMainWindow.h"
#include "MuonMainWindow.h"

// Qt includes
#include <QStringBuilder>
#include <QtGui/QLabel>
#include <QtGui/QShortcut>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KDebug>
#include <KFileDialog>
#include <KMessageBox>
#include <KProcess>
#include <KProtocolManager>
#include <KStandardDirs>
#include <KVBox>
#include <KLocalizedString>
#include <Solid/PowerManagement>
#include <Solid/Networking>
#include <Phonon/MediaObject>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/DebFile>


AptifyMainWindow::AptifyMainWindow(KXmlGuiWindow* parent)
    : QObject(parent)
    , m_backend(0)
    , m_powerInhibitor(0)
    , m_canExit(false)
    , m_isReloading(false)
    , m_actionsDisabled(false)
    , m_mainWindow(parent)
{
}

void AptifyMainWindow::initObject()
{
    m_backend = new QApt::Backend;
    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode,QVariantMap)),
            this, SLOT(errorOccurred(QApt::ErrorCode,QVariantMap)));
    connect(m_backend, SIGNAL(warningOccurred(QApt::WarningCode,QVariantMap)),
            this, SLOT(warningOccurred(QApt::WarningCode,QVariantMap)));
    connect(m_backend, SIGNAL(questionOccurred(QApt::WorkerQuestion,QVariantMap)),
            this, SLOT(questionOccurred(QApt::WorkerQuestion,QVariantMap)));
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(setActionsEnabled()));
    m_backend->init();
    m_originalState = m_backend->currentCacheState();

    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        m_backend->setWorkerProxy(KProtocolManager::proxyFor("http"));
    }

    emit backendReady(m_backend);

    m_canExit = true;
    connect(Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)), this, SLOT(networkChanged()));
}

void AptifyMainWindow::slotQuit()
{
    KApplication::instance()->quit();
}

bool AptifyMainWindow::queryExit()
{
    // We don't want to quit during the middle of a commit
    if (!m_canExit) {
        return false;
    }

    if (m_backend->areChangesMarked()) {
        QString text = i18nc("@label", "There are marked changes that have not yet "
                             "been applied. Do you want to save your changes "
                             "or discard them?");
        int res = KMessageBox::Cancel;
        res = KMessageBox::warningYesNoCancel(m_mainWindow, text, QString(), KStandardGuiItem::saveAs(),
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

void AptifyMainWindow::setupActions()
{
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), m_mainWindow->actionCollection());
    m_mainWindow->actionCollection()->addAction("quit", quitAction);

    KAction* updateAction = m_mainWindow->actionCollection()->addAction("update");
    updateAction->setIcon(KIcon("system-software-update"));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, SIGNAL(triggered()), this, SLOT(checkForUpdates()));
    if (!isConnected()) {
        updateAction->setDisabled(true);
    }
    connect(this, SIGNAL(shouldConnect(bool)), updateAction, SLOT(setEnabled(bool)));


    KAction* undoAction = KStandardAction::undo(this, SLOT(undo()), actionCollection());
    actionCollection()->addAction("undo", undoAction);

    KAction* redoAction = KStandardAction::redo(this, SLOT(redo()), actionCollection());
    actionCollection()->addAction("redo", redoAction);

    KAction* revertAction = actionCollection()->addAction("revert");
    revertAction->setIcon(KIcon("document-revert"));
    revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));

    KAction* softwarePropertiesAction = actionCollection()->addAction("software_properties");
    softwarePropertiesAction->setIcon(KIcon("configure"));
    softwarePropertiesAction->setText(i18nc("@action Opens the software sources configuration dialog", "Configure Software Sources"));
    connect(softwarePropertiesAction, SIGNAL(triggered()), this, SLOT(runSourcesEditor()));

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), m_mainWindow);
    connect(shortcut, SIGNAL(activated()), this, SLOT(easterEggTriggered()));
}

void AptifyMainWindow::checkForUpdates()
{
}

void AptifyMainWindow::workerEvent(QApt::WorkerEvent event)
{
    switch (event) {
    case QApt::CacheUpdateStarted:
        break;
    case QApt::PackageDownloadStarted:
        m_powerInhibitor = Solid::PowerManagement::beginSuppressingSleep(i18nc("@info:status", "Muon is making system changes"));
    case QApt::CommitChangesStarted:
        m_canExit = false;
        break;
    case QApt::CacheUpdateFinished:
    case QApt::PackageDownloadFinished:
    case QApt::CommitChangesFinished:
        Solid::PowerManagement::stopSuppressingSleep(m_powerInhibitor);
        m_canExit = true;
        break;
    case QApt::XapianUpdateFinished:
        m_backend->openXapianIndex();
        break;
    default:
        break;
    }
}

void AptifyMainWindow::errorOccurred(QApt::ErrorCode code, const QVariantMap &args)
{
    QString text;
    QString title;

    if (m_isReloading)
        return;

    switch (code) {
    case QApt::InitError: {
        text = i18nc("@label",
                     "The package system could not be initialized, your "
                     "configuration may be broken.");
        title = i18nc("@title:window", "Initialization error");
        QString details = args["ErrorText"].toString();
        bool fromWorker = args["FromWorker"].toBool();
        KMessageBox::detailedError(m_mainWindow, text, details, title);
        if (!fromWorker) {
            exit(-1);
        }
        break;
    }
    case QApt::LockError:
        text = i18nc("@label",
                     "Another application seems to be using the package "
                     "system at this time. You must close all other package "
                     "managers before you will be able to install or remove "
                     "any packages.");
        title = i18nc("@title:window", "Unable to obtain package system lock");
        KMessageBox::error(m_mainWindow, text, title);
        setActionsEnabled();
        break;
    case QApt::DiskSpaceError: {
        QString drive = args["DirectoryString"].toString();
        text = i18nc("@label",
                     "You do not have enough disk space in the directory "
                     "at %1 to continue with this operation.", drive);
        title = i18nc("@title:window", "Low disk space");
        KMessageBox::error(m_mainWindow, text, title);
        setActionsEnabled();
        break;
    }
    case QApt::FetchError:
        text = i18nc("@label",
                     "Changes could not be applied since some packages "
                     "could not be downloaded.");
        title = i18nc("@title:window", "Failed to Apply Changes");
        KMessageBox::error(m_mainWindow, text, title);
        break;
    case QApt::CommitError: {
        m_errorStack.append(args);
        return;
    }
    case QApt::AuthError:
        text = i18nc("@label",
                     "This operation cannot continue since proper "
                     "authorization was not provided");
        title = i18nc("@title:window", "Authentication error");
        KMessageBox::error(m_mainWindow, text, title);
        setActionsEnabled();
        break;
    case QApt::WorkerDisappeared:
        text = i18nc("@label", "It appears that the QApt worker has either crashed "
                     "or disappeared. Please report a bug to the QApt maintainers");
        title = i18nc("@title:window", "Unexpected Error");
        KMessageBox::error(m_mainWindow, text, title);
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
        KMessageBox::errorList(m_mainWindow, text, untrustedItems, title);
        setActionsEnabled();
        break;
    }
    case QApt::UserCancelError:
    case QApt::UnknownError:
    default:
        break;
    }

    m_canExit = true; // If we were committing changes, we aren't anymore
}

void AptifyMainWindow::warningOccurred(QApt::WarningCode warning, const QVariantMap &args)
{
    switch (warning) {
    case QApt::SizeMismatchWarning: {
        QString text = i18nc("@label",
                             "The size of the downloaded items did not equal the expected size.");
        QString title = i18nc("@title:window", "Size Mismatch");
        KMessageBox::sorry(m_mainWindow, text, title);
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

void AptifyMainWindow::questionOccurred(QApt::WorkerQuestion code, const QVariantMap &args)
{
    QVariantMap response;

    switch (code) {
    case QApt::ConfFilePrompt: {
        // TODO: diff support
        QString oldFile = args["OldConfFile"].toString();

        // FIXME: dpkg isn't waiting for a response within the qaptworker.
        // Re-enable once that's working
//         QString title = i18nc("@title:window", "Configuration File Changed");
//         QString text = i18nc("@label Notifies a config file change",
//                              "A new version of the configuration file "
//                              "<filename>%1</filename> is available, but your version has "
//                              "been modified. Would you like to keep your current version "
//                              "or install the new version?", oldFile);
// 
//         KGuiItem useNew(i18nc("@action Use the new config file", "Use New Version"));
//         KGuiItem useOld(i18nc("@action Keep the old config file", "Keep Old Version"));
// 
//         int ret = KMessageBox::questionYesNo(this, text, title, useNew, useOld);
// 
//         if (ret == KMessageBox::Yes) {
//             response["ReplaceFile"] = true;
//         } else {
//             response["ReplaceFile"] = false;
//         }

        m_backend->answerWorkerQuestion(response);
        break;
    }
    case QApt::MediaChange: {
        QString media = args["Media"].toString();
        QString drive = args["Drive"].toString();

        QString title = i18nc("@title:window", "Media Change Required");
        QString text = i18nc("@label Asks for a CD change", "Please insert %1 into <filename>%2</filename>", media, drive);

        KMessageBox::information(m_mainWindow, text, title);
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

        result = KMessageBox::warningContinueCancelList(m_mainWindow, text,
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

void AptifyMainWindow::showQueuedWarnings()
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
    KMessageBox::detailedError(m_mainWindow, text, details, title);
}

void AptifyMainWindow::showQueuedErrors()
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
    KMessageBox::detailedError(m_mainWindow, text, details, title);
    m_canExit = true;
}

void AptifyMainWindow::reload()
{
}

bool AptifyMainWindow::saveSelections()
{
    QString filename;

    filename = KFileDialog::getSaveFileName(QString(), QString(), m_mainWindow,
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
        KMessageBox::error(m_mainWindow, text, QString());
        return false;
    }

    return true;
}

bool AptifyMainWindow::saveInstalledPackagesList()
{
    QString filename;

    filename = KFileDialog::getSaveFileName(QString(), QString(), m_mainWindow,
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
        KMessageBox::error(m_mainWindow, text, QString());
        return false;
    }

    return true;
}

bool AptifyMainWindow::createDownloadList()
{
    QString filename;
    filename = KFileDialog::getSaveFileName(QString(), QString(), m_mainWindow,
                                            i18nc("@title:window", "Save Download List As"));

    if (filename.isEmpty()) {
        return false;
    }

    if (!m_backend->saveDownloadList(filename)) {
        QString text = i18nc("@label", "The document could not be saved, as it "
                             "was not possible to write to "
                             "<filename>%1</filename>\n\nCheck "
                             "that you have write access to this file "
                             "or that enough disk space is available.",
                             filename);
        KMessageBox::error(m_mainWindow, text, QString());
        return false;
    }

    return true;
}

void AptifyMainWindow::downloadPackagesFromList()
{
    QString filename = KFileDialog::getOpenFileName(QString(), QString(),
                                                    m_mainWindow, i18nc("@title:window", "Open File"));

    if (filename.isEmpty()) {
        return;
    }

    QString dirName = filename.left(filename.lastIndexOf('/'));

    setActionsEnabled(false);
    m_backend->downloadArchives(filename, dirName % QLatin1String("/packages"));
}

void AptifyMainWindow::loadSelections()
{
    QString filename = KFileDialog::getOpenFileName(QString(), QString(),
                                                    m_mainWindow, i18nc("@title:window", "Open File"));

    if (filename.isEmpty()) {
        return;
    }

    m_backend->saveCacheState();
    if (!m_backend->loadSelections(filename)) {
        QString text = i18nc("@label", "Could not mark changes. Please make sure "
                             "that the file is a markings file created by "
                             "either the Muon Package Manager or the "
                             "Synaptic Package Manager.");
        KMessageBox::error(m_mainWindow, text, QString());
    }
}

void AptifyMainWindow::loadArchives()
{
    QString dirName;

    dirName = KFileDialog::getExistingDirectory(KUrl(), m_mainWindow,
                                                i18nc("@title:window",
                                                      "Choose a Directory"));

    if (dirName.isEmpty()) {
        // User cancelled
        return;
    }

    QDir dir(dirName);
    QStringList archiveFiles = dir.entryList(QDir::Files, QDir::Name);

    int successCount = 0;
    foreach (const QString &archiveFile, archiveFiles) {
        const QApt::DebFile debFile(dirName % '/' % archiveFile);

        if (debFile.isValid()) {
            if (m_backend->addArchiveToCache(debFile)) {
                successCount++;
            }
        }
    }

    if (successCount) {
        QString message = i18ncp("@label",
                                 "%1 package was successfully added to the cache",
                                 "%1 packages were successfully added to the cache",
                                 successCount);
        KMessageBox::information(m_mainWindow, message, QString());
    } else {
        QString message = i18nc("@label",
                                "No valid packages could be found in this directory. "
                                "Please make sure the packages are compatible with your "
                                "computer and are at the latest version.");
        KMessageBox::error(m_mainWindow, message, i18nc("@title:window",
                                                "Packages Could Not be Found"));
    }
}

void AptifyMainWindow::undo()
{
    m_backend->undo();
}

void AptifyMainWindow::redo()
{
    m_backend->redo();
}

void AptifyMainWindow::revertChanges()
{
    m_backend->restoreCacheState(m_originalState);
}

void AptifyMainWindow::runSourcesEditor(bool update)
{
    KProcess *proc = new KProcess(this);
    QStringList arguments;
    int winID = m_mainWindow->effectiveWinId();

    QString editor = "software-properties-kde";

    if (!update) {
        editor.append(QLatin1Literal(" --dont-update --attach ") % QString::number(winID)); //krazy:exclude=spelling;
    } else {
        editor.append(QLatin1Literal(" --attach ") % QString::number(winID));
    }

    arguments << "/usr/bin/kdesudo" << editor;

    proc->setProgram(arguments);
    m_mainWindow->find(winID)->setEnabled(false);
    proc->start();
    connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(sourcesEditorFinished(int)));
}

void AptifyMainWindow::sourcesEditorFinished(int reload)
{
    m_mainWindow->find(m_mainWindow->effectiveWinId())->setEnabled(true);
    if (reload == 1) {
        checkForUpdates();
    }
}

void AptifyMainWindow::easterEggTriggered()
{
    KDialog *dialog = new KDialog(m_mainWindow);
    KVBox *widget = new KVBox(dialog);
    QLabel *label = new QLabel(widget);
    label->setText(i18nc("@label Easter Egg", "This Muon has super cow powers"));
    QLabel *moo = new QLabel(widget);
    moo->setFont(QFont("monospace"));
    moo->setText("             (__)\n"
                 "             (oo)\n"
                 "    /---------\\/\n"
                 "   / | Muuu!!||\n"
                 "  *  ||------||\n"
                 "     ^^      ^^\n");

    dialog->setMainWidget(widget);
    dialog->show();

    QString mooFile = KStandardDirs::locate("data", "libmuon/moo.ogg");
    Phonon::MediaObject *music =
        Phonon::createPlayer(Phonon::MusicCategory,
                             Phonon::MediaSource(mooFile));
    music->play();
}

bool AptifyMainWindow::isConnected() const {
    int status = Solid::Networking::status();
    bool connected = ((status == Solid::Networking::Connected) ||
                      (status == Solid::Networking::Unknown));
    return connected;
}

void AptifyMainWindow::networkChanged()
{
    if (m_actionsDisabled) {
        return;
    }

    emit shouldConnect(isConnected());
}

void AptifyMainWindow::setActionsEnabled(bool enabled)
{
    m_actionsDisabled = !enabled;
    for (int i = 0; i < actionCollection()->count(); ++i) {
        actionCollection()->action(i)->setEnabled(enabled);
    }
    actionCollection()->action("update")->setEnabled(isConnected() && enabled);
    
    if(enabled) {
        actionCollection()->action("undo")->setEnabled(!m_backend->isUndoStackEmpty());
        actionCollection()->action("redo")->setEnabled(!m_backend->isRedoStackEmpty());
        actionCollection()->action("revert")->setEnabled(!m_backend->isUndoStackEmpty());
    }
}

KActionCollection* AptifyMainWindow::actionCollection()
{
    return m_mainWindow->actionCollection();
}

void AptifyMainWindow::initializationErrors(const QString& errors)
{
    QVariantMap args;
    args["ErrorText"] = errors;
    args["FromWorker"] = false;
    errorOccurred(QApt::InitError, args);
}


#include "AptifyMainWindow.moc"
