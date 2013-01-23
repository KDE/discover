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

// Qt includes
#include <QtCore/QDir>
#include <QtCore/QStringBuilder>

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

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/DebFile>
#include <LibQApt/Transaction>

// Own includes
#include "MuonMainWindow.h"

QAptActions::QAptActions()
    : QObject(0)
    , m_backend(0)
    , m_actionsDisabled(false)
    , m_mainWindow(0)
    , m_reloadWhenEditorFinished(false)
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

void QAptActions::setMainWindow(KXmlGuiWindow* w)
{
    setParent(w);
    m_mainWindow = w;
    setupActions();
}

KXmlGuiWindow* QAptActions::mainWindow() const
{
    return m_mainWindow;
}

void QAptActions::setBackend(QApt::Backend* backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), this, SLOT(setActionsEnabled()));
    setReloadWhenEditorFinished(true);
    // Some actions need an initialized backend to be able to set their enabled state
    setActionsEnabled(true);
}

void QAptActions::setupActions()
{
    KAction* updateAction = m_mainWindow->actionCollection()->addAction("update");
    updateAction->setIcon(KIcon("system-software-update"));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, SIGNAL(triggered()), SIGNAL(checkForUpdates()));
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
    
    KAction* loadSelectionsAction = actionCollection()->addAction("open_markings");
    loadSelectionsAction->setIcon(KIcon("document-open"));
    loadSelectionsAction->setText(i18nc("@action", "Read Markings..."));
    connect(loadSelectionsAction, SIGNAL(triggered()), this, SLOT(loadSelections()));

    KAction* saveSelectionsAction = actionCollection()->addAction("save_markings");
    saveSelectionsAction->setIcon(KIcon("document-save-as"));
    saveSelectionsAction->setText(i18nc("@action", "Save Markings As..."));
    connect(saveSelectionsAction, SIGNAL(triggered()), this, SLOT(saveSelections()));

    KAction* createDownloadListAction = actionCollection()->addAction("save_download_list");
    createDownloadListAction->setIcon(KIcon("document-save-as"));
    createDownloadListAction->setText(i18nc("@action", "Save Package Download List..."));
    connect(createDownloadListAction, SIGNAL(triggered()), this, SLOT(createDownloadList()));

    KAction* downloadListAction = actionCollection()->addAction("download_from_list");
    downloadListAction->setIcon(KIcon("download"));
    downloadListAction->setText(i18nc("@action", "Download Packages From List..."));
    connect(downloadListAction, SIGNAL(triggered()), this, SLOT(downloadPackagesFromList()));
    if (!isConnected()) {
        downloadListAction->setDisabled(false);
    }
    connect(this, SIGNAL(shouldConnect(bool)), downloadListAction, SLOT(setEnabled(bool)));

    KAction* loadArchivesAction = actionCollection()->addAction("load_archives");
    loadArchivesAction->setIcon(KIcon("document-open"));
    loadArchivesAction->setText(i18nc("@action", "Add Downloaded Packages"));
    connect(loadArchivesAction, SIGNAL(triggered()), this, SLOT(loadArchives()));
    
    KAction* saveInstalledAction = actionCollection()->addAction("save_package_list");
    saveInstalledAction->setIcon(KIcon("document-save-as"));
    saveInstalledAction->setText(i18nc("@action", "Save Installed Packages List..."));
    connect(saveInstalledAction, SIGNAL(triggered()), this, SLOT(saveInstalledPackagesList()));
}

void QAptActions::setActionsEnabled(bool enabled)
{
    m_actionsDisabled = !enabled;
    for (int i = 0; i < actionCollection()->count(); ++i) {
        actionCollection()->action(i)->setEnabled(enabled);
    }

    if (!enabled)
        return;

    actionCollection()->action("update")->setEnabled(isConnected() && enabled);

    actionCollection()->action("undo")->setEnabled(!m_backend->isUndoStackEmpty());
    actionCollection()->action("redo")->setEnabled(!m_backend->isRedoStackEmpty());
    actionCollection()->action("revert")->setEnabled(!m_backend->isUndoStackEmpty());
    
    actionCollection()->action("save_download_list")->setEnabled(isConnected());

    bool changesPending = m_backend->areChangesMarked();
    actionCollection()->action("open_markings")->setEnabled(true);
    actionCollection()->action("save_markings")->setEnabled(changesPending);
    actionCollection()->action("save_download_list")->setEnabled(changesPending);
    actionCollection()->action("save_package_list")->setEnabled(true);
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
        emit checkForUpdates();
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
