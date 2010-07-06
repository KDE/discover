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

#include "MainWindow.h"

// Qt includes
#include <QStringBuilder>
#include <QtCore/QTimer>
#include <QtGui/QLabel>
#include <QtGui/QShortcut>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBox>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KConfigDialog>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KStandardAction>
#include <KStatusBar>
#include <Solid/PowerManagement>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "CommitWidget.h"
#include "DownloadWidget.h"
#include "FilterWidget.h"
#include "ManagerWidget.h"
#include "ReviewWidget.h"
#include "StatusWidget.h"

MainWindow::MainWindow()
    : KXmlGuiWindow(0)
    , m_backend(0)
    , m_stack(0)
    , m_reviewWidget(0)
    , m_downloadWidget(0)
    , m_commitWidget(0)
    , m_powerInhibitor(0)
    , m_canExit(true)

{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::initGUI()
{
    m_stack = new QStackedWidget;
    setCentralWidget(m_stack);

    m_managerWidget = new ManagerWidget(m_stack);
    connect (this, SIGNAL(backendReady(QApt::Backend*)),
             m_managerWidget, SLOT(setBackend(QApt::Backend*)));

    m_mainWidget = new QSplitter(this);
    m_mainWidget->setOrientation(Qt::Horizontal);

    m_filterBox = new FilterWidget(m_stack);
    connect (this, SIGNAL(backendReady(QApt::Backend*)),
             m_filterBox, SLOT(setBackend(QApt::Backend*)));
    connect (m_filterBox, SIGNAL(filterByGroup(const QString&)),
             m_managerWidget, SLOT(filterByGroup(const QString&)));
    connect (m_filterBox, SIGNAL(filterByStatus(const QString&)),
             m_managerWidget, SLOT(filterByStatus(const QString&)));

    m_mainWidget->addWidget(m_filterBox);
    m_mainWidget->addWidget(m_managerWidget);
    // TODO: Store/restore on app exit/restore
    QList<int> sizes;
    sizes << 115 << (this->width() - 115);
    m_mainWidget->setSizes(sizes);

    m_stack->addWidget(m_mainWidget);
    m_stack->setCurrentWidget(m_mainWidget);

    setupActions();

    m_statusWidget = new StatusWidget(this);
    connect (this, SIGNAL(backendReady(QApt::Backend*)),
             m_statusWidget, SLOT(setBackend(QApt::Backend*)));
    statusBar()->addWidget(m_statusWidget);
    statusBar()->show();
}

void MainWindow::initObject()
{
    m_backend = new QApt::Backend;
    m_backend->init();
    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode, const QVariantMap&)),
            this, SLOT(errorOccurred(QApt::ErrorCode, const QVariantMap&)));
    connect(m_backend, SIGNAL(warningOccurred(QApt::WarningCode, const QVariantMap&)),
            this, SLOT(warningOccurred(QApt::WarningCode, const QVariantMap&)));
    connect(m_backend, SIGNAL(questionOccurred(QApt::WorkerQuestion, const QVariantMap&)),
            this, SLOT(questionOccurred(QApt::WorkerQuestion, const QVariantMap&)));
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(reloadActions()));

    reloadActions(); //Get initial enabled/disabled state

    m_managerWidget->setFocus();

    emit backendReady(m_backend);
}

void MainWindow::setupActions()
{
    // local - Destroys all sub-windows and exits
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);

    m_updateAction = actionCollection()->addAction("update");
    m_updateAction->setIcon(KIcon("system-software-update"));
    m_updateAction->setText(i18nc("@action Checks the internet for updates", "Check for Updates"));
    m_updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(m_updateAction, SIGNAL(triggered()), this, SLOT(checkForUpdates()));

    m_safeUpgradeAction = actionCollection()->addAction("safeupgrade");
    m_safeUpgradeAction->setIcon(KIcon("go-up"));
    m_safeUpgradeAction->setText(i18nc("@action Marks upgradeable packages for upgrade", "Cautious Upgrade"));
    connect(m_safeUpgradeAction, SIGNAL(triggered()), this, SLOT(markUpgrade()));

    m_distUpgradeAction = actionCollection()->addAction("fullupgrade");
    m_distUpgradeAction->setIcon(KIcon("go-top"));
    m_distUpgradeAction->setText(i18nc("@action Marks upgradeable packages, including ones that install/remove new things",
                                   "Full Upgrade"));
    connect(m_distUpgradeAction, SIGNAL(triggered()), this, SLOT(markDistUpgrade()));

    m_previewAction = actionCollection()->addAction("preview");
    m_previewAction->setIcon(KIcon("document-preview-archive"));
    m_previewAction->setText(i18nc("@action Takes the user to the preview page", "Preview Changes"));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));

    m_applyAction = actionCollection()->addAction("apply");
    m_applyAction->setIcon(KIcon("dialog-ok-apply"));
    m_applyAction->setText(i18nc("@action Applys the changes a user has made", "Apply Changes"));
    connect(m_applyAction, SIGNAL(triggered()), this, SLOT(startCommit()));

    m_undoAction = KStandardAction::undo(this, SLOT(undo()), actionCollection());
    actionCollection()->addAction("undo", m_undoAction);

    m_redoAction = KStandardAction::redo(this, SLOT(redo()), actionCollection());
    actionCollection()->addAction("redo", m_redoAction);

    m_revertAction = actionCollection()->addAction("revert");
    m_revertAction->setIcon(KIcon("document-revert"));
    m_revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));

    KAction *action = actionCollection()->addAction("software_properties");
    action->setIcon(KIcon("configure"));
    action->setText(i18nc("@action Opens the software sources configuration dialog", "Configure Software Sources"));
    connect(action, SIGNAL(triggered()), this, SLOT(runSourcesEditor()));

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(easterEggTriggered()));

    setupGUI();
}

void MainWindow::slotQuit()
{
    //Settings::self()->writeConfig();
    if (queryExit()) {
        KApplication::instance()->quit();
    }
}

bool MainWindow::queryExit()
{
    // We don't want to quit during the middle of a commit
    return m_canExit;
}

void MainWindow::markUpgrade()
{
    m_backend->saveCacheState();
    m_backend->markPackagesForUpgrade();

    if(m_backend-> markedPackages().isEmpty()) {
         QString text = i18nc("@label", "Unable to mark upgrades. The "
                              "available upgrades may require new packages to "
                              "be installed or removed. You may wish to try "
                              "a full upgrade by clicking the <interface>Full "
                              " Upgrade</interface> button.");
         QString title = i18nc("@title:window", "Unable to Mark Upgrades");
         KMessageBox::information(this, text, title);
    } else {
        previewChanges();
    }
}

void MainWindow::markDistUpgrade()
{
    m_backend->saveCacheState();
    m_backend->markPackagesForDistUpgrade();
    if(m_backend-> markedPackages().isEmpty()) {
         QString text = i18nc("@label", "Unable to mark upgrades. Some "
                              "upgrades may have unsatisfiable dependencies at "
                              "the moment, or may have been manually held back.");
         QString title = i18nc("@title:window", "Unable to Mark Upgrades");
         KMessageBox::information(this, text, title);
    } else {
        previewChanges();
    }
}

void MainWindow::checkForUpdates()
{
    setActionsEnabled(false);
    initDownloadWidget();
    m_backend->updateCache();
}

void MainWindow::workerEvent(QApt::WorkerEvent event)
{
    switch (event) {
        case QApt::CacheUpdateStarted:
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Updating software sources</title>"));
            m_stack->setCurrentWidget(m_downloadWidget);
            m_powerInhibitor = Solid::PowerManagement::beginSuppressingSleep(i18nc("@info:status", "Muon is downloading packages"));
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::CacheUpdateFinished:
        case QApt::CommitChangesFinished:
            Solid::PowerManagement::stopSuppressingSleep(m_powerInhibitor);
            m_canExit = true;
            reload();
            returnFromPreview();
            break;
        case QApt::PackageDownloadStarted:
            m_downloadWidget->setHeaderText(i18nc("@info", "<title>Downloading Packages</title>"));
            m_powerInhibitor = Solid::PowerManagement::beginSuppressingSleep(i18nc("@info:status", "Muon is downloading packages"));
            m_stack->setCurrentWidget(m_downloadWidget);
            connect(m_downloadWidget, SIGNAL(cancelDownload()), m_backend, SLOT(cancelDownload()));
            break;
        case QApt::CommitChangesStarted:
            m_canExit = false;
            m_commitWidget->setHeaderText(i18nc("@info", "<title>Committing Changes</title>"));
            m_stack->setCurrentWidget(m_commitWidget);
            break;
        case QApt::PackageDownloadFinished:
        case QApt::InvalidEvent:
        default:
            break;
    }
}

void MainWindow::errorOccurred(QApt::ErrorCode code, const QVariantMap &args)
{
    QString text;
    QString title;

    switch(code) {
        case QApt::InitError:
            text = i18nc("@label",
                         "The package system could not be initialized, your "
                         "configuration may be broken.");
            title = i18nc("@title:window", "Initialization error");
            KMessageBox::error(this, text, title);
            break;
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
            QString failedItem = i18nc("@label Shows which package failed", "Package: %1", args["FailedItem"].toString());
            QString errorText = i18nc("@label Shows the error", "Error: %1", args["ErrorText"].toString());

            text = i18nc("@label", "An error occurred while committing changes.");
            QString details = QString(failedItem % "\n\n" % errorText);
            title = i18nc("@title:window", "Commit error");
            kDebug() << details;

            KMessageBox::detailedError(this, text, details, title);
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
    returnFromPreview(); // Change the "back" button back to normal in case we were in preview
    m_stack->setCurrentWidget(m_mainWidget);
}

void MainWindow::warningOccurred(QApt::WarningCode warning, const QVariantMap &args)
{
    switch (warning) {
        case QApt::SizeMismatchWarning:
            // TODO
            break;
        case QApt::FetchFailedWarning: {
            QString failedItem = args["FailedItem"].toString();
            QString warningText = args["WarningText"].toString();
            QString text = i18nc("@label",
                                 "Failed to download %1\n"
                                 "%2", failedItem, warningText);
            QString title = i18nc("@title:window", "Download Failed");
            KMessageBox::sorry(this, text, title);
            break;
        }
        case QApt::UnknownWarning:
        default:
            break;
    }

}

void MainWindow::questionOccurred(QApt::WorkerQuestion code, const QVariantMap &args)
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
                    reloadActions(); //Pseudo-error in this case. Reset things
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

void MainWindow::previewChanges()
{
    m_reviewWidget = new ReviewWidget(m_stack);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_reviewWidget, SLOT(setBackend(QApt::Backend*)));
    m_reviewWidget->setBackend(m_backend);
    m_stack->addWidget(m_reviewWidget);

    m_stack->setCurrentWidget(m_reviewWidget);

    m_previewAction->setIcon(KIcon("go-previous"));
    m_previewAction->setText(i18nc("@action:intoolbar Return from the preview page", "Back"));
    disconnect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(returnFromPreview()));
}

void MainWindow::returnFromPreview()
{
    m_stack->setCurrentWidget(m_mainWidget);
    m_reviewWidget->deleteLater();
    m_reviewWidget = 0;

    m_previewAction->setIcon(KIcon("document-preview-archive"));
    m_previewAction->setText(i18nc("@action", "Preview Changes"));
    disconnect(m_previewAction, SIGNAL(triggered()), this, SLOT(returnFromPreview()));
    connect(m_previewAction, SIGNAL(triggered()), this, SLOT(previewChanges()));
    // We may not have anything to preview; check.
    reloadActions(); 
}

void MainWindow::startCommit()
{
    setActionsEnabled(false);
    initDownloadWidget();
    initCommitWidget();
    m_backend->commitChanges();
}

void MainWindow::initDownloadWidget()
{
    if (!m_downloadWidget) {
        m_downloadWidget = new DownloadWidget(this);
        m_stack->addWidget(m_downloadWidget);
        connect(m_backend, SIGNAL(downloadProgress(int, int, int)),
                m_downloadWidget, SLOT(updateDownloadProgress(int, int, int)));
        connect(m_backend, SIGNAL(downloadMessage(int, const QString&)),
                m_downloadWidget, SLOT(updateDownloadMessage(int, const QString&)));
    }
}

void MainWindow::initCommitWidget()
{
    if (!m_commitWidget) {
        m_commitWidget = new CommitWidget(this);
        m_stack->addWidget(m_commitWidget);
        connect(m_backend, SIGNAL(commitProgress(const QString&, int)),
                m_commitWidget, SLOT(updateCommitProgress(const QString&, int)));
    }
}

void MainWindow::reload()
{
    m_managerWidget->reload();
    if (m_reviewWidget) {
        m_reviewWidget->refresh();
    }
    m_statusWidget->updateStatus();
    setActionsEnabled(true);
    reloadActions();

    // No need to keep these around in memory.
    m_downloadWidget->deleteLater();
    m_commitWidget->deleteLater();
    m_downloadWidget = 0;
    m_commitWidget = 0;
}

void MainWindow::reloadActions()
{
    int upgradeable = m_backend->packageCount(QApt::Package::Upgradeable);
    QApt::PackageList changedList = m_backend->markedPackages();

    m_updateAction->setEnabled(true);
    m_safeUpgradeAction->setEnabled(upgradeable > 0);
    m_distUpgradeAction->setEnabled(upgradeable > 0);
    if (m_stack->currentWidget() == m_reviewWidget) {
        // We always need to be able to get back from review
        m_previewAction->setEnabled(true);
    } else {
        m_previewAction->setEnabled(!changedList.isEmpty());
    }
    m_applyAction->setEnabled(!changedList.isEmpty());

    m_undoAction->setEnabled(!m_backend->isUndoStackEmpty());
    m_redoAction->setEnabled(!m_backend->isRedoStackEmpty());
    m_revertAction->setEnabled(!m_backend->isUndoStackEmpty());
}

void MainWindow::setActionsEnabled(bool enabled)
{
    m_updateAction->setEnabled(enabled);
    m_safeUpgradeAction->setEnabled(enabled);
    m_distUpgradeAction->setEnabled(enabled);
    m_previewAction->setEnabled(enabled);
    m_applyAction->setEnabled(enabled);
    m_undoAction->setEnabled(enabled);
    m_redoAction->setEnabled(enabled);
    m_revertAction->setEnabled(enabled);
}

void MainWindow::undo()
{
    m_backend->undo();
}

void MainWindow::redo()
{
    m_backend->redo();
}

void MainWindow::revertChanges()
{
    m_backend->init();
    reload();
}

void MainWindow::runSourcesEditor()
{
    KProcess* proc = new KProcess(this);
    QStringList arguments;
    QString cmd;
    int winID = effectiveWinId();
    cmd = "software-properties-kde --dont-update --attach " + QString::number(winID);
    arguments << "/usr/lib/kde4/libexec/kdesu" << QString(cmd);
    proc->setProgram(arguments);
    find(winID)->setEnabled(false);
    proc->start();
    connect( proc, SIGNAL( finished(int, QProcess::ExitStatus) ),
             this, SLOT( sourcesEditorFinished(int) ) );
}

void MainWindow::sourcesEditorFinished(int reload)
{
    find(effectiveWinId())->setEnabled(true);
    if(reload == 1) {
        checkForUpdates();
    }
}

void MainWindow::easterEggTriggered()
{
    KDialog *dialog = new KDialog(this);
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
}

#include "MainWindow.moc"
