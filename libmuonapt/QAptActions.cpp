/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "QAptActions.h"
#include "MuonStrings.h"
#include "HistoryView/HistoryView.h"

// Qt includes
#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QDebug>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KFileDialog>
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KStandardAction>
#include <KXmlGuiWindow>
#include <Solid/Networking>
#include <KStandardDirs>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/DebFile>
#include <LibQApt/Transaction>

// Own includes
#include "MuonMainWindow.h"

QAptActions::QAptActions()
    : QObject(nullptr)
    , m_backend(nullptr)
    , m_actionsDisabled(false)
    , m_mainWindow(nullptr)
    , m_reloadWhenEditorFinished(false)
    , m_historyDialog(nullptr)
{
    connect(Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
            this, SLOT(networkChanged()));
}

QAptActions* QAptActions::self()
{
    static QWeakPointer<QAptActions> self;
    if(!self) {
        self = new QAptActions;
    }
    return self.data();
}

void QAptActions::setMainWindow(MuonMainWindow* w)
{
    setParent(w);
    m_mainWindow = w;
    connect(m_mainWindow, SIGNAL(actionsEnabledChanged(bool)), SLOT(setActionsEnabledInternal(bool)));
    setupActions();
}

MuonMainWindow* QAptActions::mainWindow() const
{
    return m_mainWindow;
}

void QAptActions::setBackend(QApt::Backend* backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), SLOT(setActionsEnabled()));
    setOriginalState(m_backend->currentCacheState());

    setReloadWhenEditorFinished(true);
    // Some actions need an initialized backend to be able to set their enabled state
    setActionsEnabled(true);
}

void QAptActions::setupActions()
{
    KAction* undoAction = KStandardAction::undo(this, SLOT(undo()), actionCollection());
    actionCollection()->addAction("undo", undoAction);
    m_actions.append(undoAction);

    KAction* redoAction = KStandardAction::redo(this, SLOT(redo()), actionCollection());
    actionCollection()->addAction("redo", redoAction);
    m_actions.append(redoAction);

    KAction* revertAction = actionCollection()->addAction("revert");
    revertAction->setIcon(KIcon("document-revert"));
    revertAction->setText(i18nc("@action Reverts all potential changes to the cache", "Unmark All"));
    connect(revertAction, SIGNAL(triggered()), this, SLOT(revertChanges()));
    m_actions.append(revertAction);

    KAction* softwarePropertiesAction = actionCollection()->addAction("software_properties");
    softwarePropertiesAction->setPriority(QAction::LowPriority);
    softwarePropertiesAction->setIcon(KIcon("configure"));
    softwarePropertiesAction->setText(i18nc("@action Opens the software sources configuration dialog", "Configure Software Sources"));
    connect(softwarePropertiesAction, SIGNAL(triggered()), this, SLOT(runSourcesEditor()));
    m_actions.append(softwarePropertiesAction);
    
    KAction* loadSelectionsAction = actionCollection()->addAction("open_markings");
    loadSelectionsAction->setIcon(KIcon("document-open"));
    loadSelectionsAction->setText(i18nc("@action", "Read Markings..."));
    connect(loadSelectionsAction, SIGNAL(triggered()), this, SLOT(loadSelections()));
    m_actions.append(loadSelectionsAction);

    KAction* saveSelectionsAction = actionCollection()->addAction("save_markings");
    saveSelectionsAction->setIcon(KIcon("document-save-as"));
    saveSelectionsAction->setText(i18nc("@action", "Save Markings As..."));
    connect(saveSelectionsAction, SIGNAL(triggered()), this, SLOT(saveSelections()));
    m_actions.append(saveSelectionsAction);

    KAction* createDownloadListAction = actionCollection()->addAction("save_download_list");
    createDownloadListAction->setPriority(QAction::LowPriority);
    createDownloadListAction->setIcon(KIcon("document-save-as"));
    createDownloadListAction->setText(i18nc("@action", "Save Package Download List..."));
    connect(createDownloadListAction, SIGNAL(triggered()), this, SLOT(createDownloadList()));
    m_actions.append(createDownloadListAction);

    KAction* downloadListAction = actionCollection()->addAction("download_from_list");
    downloadListAction->setPriority(QAction::LowPriority);
    downloadListAction->setIcon(KIcon("download"));
    downloadListAction->setText(i18nc("@action", "Download Packages From List..."));
    connect(downloadListAction, SIGNAL(triggered()), this, SLOT(downloadPackagesFromList()));
    if (!isConnected()) {
        downloadListAction->setDisabled(false);
    }
    connect(this, SIGNAL(shouldConnect(bool)), downloadListAction, SLOT(setEnabled(bool)));
    m_actions.append(downloadListAction);

    KAction* loadArchivesAction = actionCollection()->addAction("load_archives");
    loadArchivesAction->setPriority(QAction::LowPriority);
    loadArchivesAction->setIcon(KIcon("document-open"));
    loadArchivesAction->setText(i18nc("@action", "Add Downloaded Packages"));
    connect(loadArchivesAction, SIGNAL(triggered()), this, SLOT(loadArchives()));
    m_actions.append(loadArchivesAction);
    
    KAction* saveInstalledAction = actionCollection()->addAction("save_package_list");
    saveInstalledAction->setPriority(QAction::LowPriority);
    saveInstalledAction->setIcon(KIcon("document-save-as"));
    saveInstalledAction->setText(i18nc("@action", "Save Installed Packages List..."));
    connect(saveInstalledAction, SIGNAL(triggered()), this, SLOT(saveInstalledPackagesList()));
    
    KAction* historyAction = actionCollection()->addAction("history");
    historyAction->setPriority(QAction::LowPriority);
    historyAction->setIcon(KIcon("view-history"));
    historyAction->setText(i18nc("@action::inmenu", "History..."));
    historyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showHistoryDialog()));

    KAction *distUpgradeAction = actionCollection()->addAction("dist-upgrade");
    distUpgradeAction->setIcon(KIcon("system-software-update"));
    distUpgradeAction->setText(i18nc("@action", "Upgrade"));
    distUpgradeAction->setPriority(QAction::HighPriority);
    distUpgradeAction->setWhatsThis(i18nc("Notification when a new version of Kubuntu is available",
                                        "A new version of Kubuntu is available."));
    distUpgradeAction->setEnabled(false);
    connect(distUpgradeAction, SIGNAL(triggered(bool)), SLOT(launchDistUpgrade()));
    checkDistUpgrade();

    m_actions.append(saveInstalledAction);
}

void QAptActions::setActionsEnabledInternal(bool enabled)
{
    m_actionsDisabled = !enabled;

    for (KAction *action : m_actions) {
        action->setEnabled(enabled);
    }

    if (!enabled)
        return;

    actionCollection()->action("update")->setEnabled(isConnected() && enabled);

    actionCollection()->action("undo")->setEnabled(m_backend && !m_backend->isUndoStackEmpty());
    actionCollection()->action("redo")->setEnabled(m_backend && !m_backend->isRedoStackEmpty());
    actionCollection()->action("revert")->setEnabled(m_backend && !m_backend->isUndoStackEmpty());
    
    actionCollection()->action("save_download_list")->setEnabled(isConnected());

    bool changesPending = m_backend && m_backend->areChangesMarked();
    actionCollection()->action("save_markings")->setEnabled(changesPending);
    actionCollection()->action("save_download_list")->setEnabled(changesPending);
    actionCollection()->action("dist-upgrade")->setEnabled(false);
    checkDistUpgrade();
}

bool QAptActions::isConnected() const {
    int status = Solid::Networking::status();
    bool connected = ((status == Solid::Networking::Connected) ||
                      (status == Solid::Networking::Unknown));
    return connected;
}

void QAptActions::networkChanged()
{
    if (m_actionsDisabled)
        return;

    emit shouldConnect(isConnected());
}

bool QAptActions::saveSelections()
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

bool QAptActions::saveInstalledPackagesList()
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

bool QAptActions::createDownloadList()
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

void QAptActions::downloadPackagesFromList()
{
    QString filename = KFileDialog::getOpenFileName(QString(), QString(),
                                                    m_mainWindow, i18nc("@title:window", "Open File"));

    if (filename.isEmpty()) {
        return;
    }

    QString dirName = filename.left(filename.lastIndexOf('/'));

    setActionsEnabled(false);
    QApt::Transaction *trans = m_backend->downloadArchives(filename, dirName % QLatin1String("/packages"));

    if (trans)
        emit downloadArchives(trans);
}

void QAptActions::loadSelections()
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

void QAptActions::loadArchives()
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

void QAptActions::undo()
{
    m_backend->undo();
}

void QAptActions::redo()
{
    m_backend->redo();
}

void QAptActions::revertChanges()
{
    m_backend->restoreCacheState(m_originalState);
    emit changesReverted();
}

void QAptActions::runSourcesEditor()
{
    KProcess *proc = new KProcess(this);
    QStringList arguments;
    int winID = m_mainWindow->effectiveWinId();

    QString editor = "software-properties-kde";

    if (m_reloadWhenEditorFinished) {
        editor.append(QLatin1String(" --dont-update --attach ") % QString::number(winID)); //krazy:exclude=spelling;
    } else {
        editor.append(QLatin1String(" --attach ") % QString::number(winID));
    }

    arguments << "/usr/bin/kdesudo" << editor;

    proc->setProgram(arguments);
    m_mainWindow->find(winID)->setEnabled(false);
    proc->start();
    connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(sourcesEditorFinished(int)));
}

void QAptActions::sourcesEditorFinished(int exitStatus)
{
    bool reload = (exitStatus == 0);
    m_mainWindow->find(m_mainWindow->effectiveWinId())->setEnabled(true);
    if (m_reloadWhenEditorFinished && reload) {
        actionCollection()->action("update")->trigger();
    }

    emit sourcesEditorClosed(reload);
}

KActionCollection* QAptActions::actionCollection()
{
    return m_mainWindow->actionCollection();
}

void QAptActions::setOriginalState(QApt::CacheState state)
{
    m_originalState = state;
}

void QAptActions::setReloadWhenEditorFinished(bool reload)
{
    m_reloadWhenEditorFinished = reload;
}

void QAptActions::initError()
{
    QString details = m_backend->initErrorMessage();

    MuonStrings *muonStrings = MuonStrings::global();

    QString title = muonStrings->errorTitle(QApt::InitError);
    QString text = muonStrings->errorText(QApt::InitError, nullptr);

    KMessageBox::detailedError(m_mainWindow, text, details, title);
    exit(-1);
}

void QAptActions::displayTransactionError(QApt::ErrorCode error, QApt::Transaction* trans)
{
    if (error == QApt::Success)
        return;

    MuonStrings *muonStrings = MuonStrings::global();

    QString title = muonStrings->errorTitle(error);
    QString text = muonStrings->errorText(error, trans);

    switch (error) {
        case QApt::InitError:
        case QApt::FetchError:
        case QApt::CommitError:
            KMessageBox::detailedError(QAptActions::self()->mainWindow(), text, trans->errorDetails(), title);
            break;
        default:
            KMessageBox::error(QAptActions::self()->mainWindow(), text, title);
            break;
    }
}

void QAptActions::showHistoryDialog()
{
    if (!m_historyDialog) {
        m_historyDialog = new KDialog(mainWindow());

        KConfigGroup dialogConfig(KSharedConfig::openConfig("muonrc"),
                                  "HistoryDialog");
        m_historyDialog->restoreDialogSize(dialogConfig);

        connect(m_historyDialog, SIGNAL(finished()), SLOT(closeHistoryDialog()));
        HistoryView *historyView = new HistoryView(m_historyDialog);
        m_historyDialog->setMainWidget(historyView);
        m_historyDialog->setWindowTitle(i18nc("@title:window", "Package History"));
        m_historyDialog->setWindowIcon(KIcon("view-history"));
        m_historyDialog->setButtons(KDialog::Close);
        m_historyDialog->show();
    } else {
        m_historyDialog->raise();
    }
}

void QAptActions::closeHistoryDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("muonrc"), "HistoryDialog");
    m_historyDialog->saveDialogSize(dialogConfig, KConfigBase::Persistent);
    m_historyDialog->deleteLater();
    m_historyDialog = nullptr;
}

void QAptActions::setActionsEnabled(bool enabled)
{
    if(m_mainWindow)
        m_mainWindow->setActionsEnabled(enabled);
    else
        setActionsEnabledInternal(enabled);
}

void QAptActions::launchDistUpgrade()
{
    KProcess *proc = new KProcess(this);
    QStringList arguments;
    QString kdesudo = KStandardDirs::findExe("kdesudo");
    QString upgrader = QString("do-release-upgrade -m desktop -f DistUpgradeViewKDE");

    arguments << kdesudo << upgrader;
    proc->setProgram(arguments);
    proc->start();

    connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
}

void QAptActions::checkDistUpgrade()
{
    if(!QFile::exists("/usr/lib/python3/dist-packages/DistUpgrade/DistUpgradeFetcherKDE.py")) {
        qWarning() << "Couldn't find the /usr/lib/python3/dist-packages/DistUpgrade/DistUpgradeFetcherKDE.py file";
    }
    QString checkerFile = KStandardDirs::locate("data", "muon-notifier/releasechecker");
    if(checkerFile.isEmpty()) {
        qWarning() << "Couldn't find the releasechecker script";
    }

    KProcess* checkerProcess = new KProcess(this);
    checkerProcess->setProgram(QStringList() << "/usr/bin/python3" << checkerFile);
    connect(checkerProcess, SIGNAL(finished(int)), this, SLOT(checkerFinished(int)));
    connect(checkerProcess, SIGNAL(finished(int)), checkerProcess, SLOT(deleteLater()));
    checkerProcess->start();
}

void QAptActions::checkerFinished(int res)
{
    QAptActions::self()->actionCollection()->action("dist-upgrade")->setEnabled(res == 0);
}
