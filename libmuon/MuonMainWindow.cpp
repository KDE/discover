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
#include "QAptIntegration.h"
#include <LibQApt/Backend>

MuonMainWindow::MuonMainWindow()
    : KXmlGuiWindow(0)
    , m_aptify(new QAptIntegration(this))
    , m_backend(m_aptify->m_backend)
    , m_canExit(m_aptify->m_canExit)
    , m_isReloading(m_aptify->m_isReloading)
    , m_originalState(m_aptify->m_originalState)
    , m_warningStack(m_aptify->m_warningStack)
    , m_errorStack(m_aptify->m_errorStack)
{
    connect(m_aptify, SIGNAL(backendReady(QApt::Backend*)), SIGNAL(backendReady(QApt::Backend*)));
    connect(m_aptify, SIGNAL(shouldConnect(bool)), SIGNAL(shouldConnect(bool)));
    connect(m_aptify, SIGNAL(checkForUpdates()), SLOT(checkForUpdates()));
}

bool MuonMainWindow::queryExit()
{
    return m_aptify->queryExit();
}

QSize MuonMainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(900, 500));
}

void MuonMainWindow::setupActions()
{
    m_aptify->setupActions();
}

void MuonMainWindow::initializationErrors(const QString& errors)
{
    m_aptify->initializationErrors(errors);
}

bool MuonMainWindow::isConnected()
{
    return m_aptify->isConnected();
}

void MuonMainWindow::revertChanges()
{
    return m_aptify->revertChanges();
}

void MuonMainWindow::setActionsEnabled(bool enabled)
{
    m_aptify->setActionsEnabled(enabled);
}

void MuonMainWindow::initObject()
{
    m_aptify->initObject();
}

void MuonMainWindow::workerEvent(QApt::WorkerEvent event)
{
    m_aptify->workerEvent(event);
}

void MuonMainWindow::showQueuedErrors()
{
    m_aptify->showQueuedErrors();
}

void MuonMainWindow::showQueuedWarnings()
{
    m_aptify->showQueuedWarnings();
}

void MuonMainWindow::errorOccurred(QApt::ErrorCode code, const QVariantMap& args)
{
    m_aptify->errorOccurred(code, args);
}

void MuonMainWindow::questionOccurred(QApt::WorkerQuestion question, const QVariantMap& details)
{
    m_aptify->questionOccurred(question, details);
}

void MuonMainWindow::warningOccurred(QApt::WarningCode warning, const QVariantMap& args)
{
    m_aptify->warningOccurred(warning, args);
}

void MuonMainWindow::downloadPackagesFromList()
{
    m_aptify->downloadPackagesFromList();
}

void MuonMainWindow::easterEggTriggered()
{
    m_aptify->easterEggTriggered();
}

void MuonMainWindow::runSourcesEditor(bool update)
{
    m_aptify->runSourcesEditor(update);
}

void MuonMainWindow::sourcesEditorFinished(int reload)
{
    m_aptify->sourcesEditorFinished(reload);
}

bool MuonMainWindow::createDownloadList()
{
    return m_aptify->createDownloadList();
}

void MuonMainWindow::loadSelections()
{
    m_aptify->loadSelections();
}

bool MuonMainWindow::saveInstalledPackagesList()
{
    return m_aptify->saveInstalledPackagesList();
}

bool MuonMainWindow::saveSelections()
{
    return m_aptify->saveSelections();
}

void MuonMainWindow::loadArchives()
{
    m_aptify->loadArchives();
}

void MuonMainWindow::checkForUpdates()
{
    m_backend->updateCache();
}
