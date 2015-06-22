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

#ifndef QAPTACTIONS_H
#define QAPTACTIONS_H

#include <QtCore/QObject>
#include <QPointer>

#include <QApt/Globals>

#include "libMuonCommon_export.h"

class MuonMainWindow;
class KDialog;
class KXmlGuiWindow;
class KActionCollection;
class QAction;
class QDialog;
class QNetworkConfigurationManager;

namespace QApt {
    class Backend;
    class Transaction;
}

class MUONCOMMON_EXPORT QAptActions : public QObject
{
    Q_OBJECT
public:
    static QAptActions* self();
    void setMainWindow(MuonMainWindow* w);
    MuonMainWindow* mainWindow() const;

    bool reloadWhenSourcesEditorFinished() const;
    bool isConnected() const;
    void setOriginalState(QApt::CacheState state);
    void setReloadWhenEditorFinished(bool reload);
    void initError();
    void displayTransactionError(QApt::ErrorCode error, QApt::Transaction* trans);
    KActionCollection* actionCollection();
    
    void setCanExit(bool e) { m_canExit = e; }
    bool canExit() const { return m_canExit; }

signals:
    void shouldConnect(bool isConnected);
    void changesReverted();
    void sourcesEditorClosed(bool reload);
    void downloadArchives(QApt::Transaction *trans);
    
public slots:
    void setBackend(QApt::Backend *backend);
    void setupActions();

    // KAction slots
    bool saveSelections();
    bool saveInstalledPackagesList();
    void loadSelections();
    bool createDownloadList();
    void downloadPackagesFromList();
    void loadArchives();
    void undo();
    void redo();
    void revertChanges();
    void runSourcesEditor();
    void sourcesEditorFinished(int exitStatus);
    void showHistoryDialog();
    void setActionsEnabled(bool enabled = true);

private slots:
    void closeHistoryDialog();
    void setActionsEnabledInternal(bool enabled);
    void checkDistUpgrade();
    void launchDistUpgrade();
    void checkerFinished(int res);

private:
    QAptActions();
    
    QApt::Backend *m_backend;
    QApt::CacheState m_originalState;
    bool m_actionsDisabled;
    MuonMainWindow* m_mainWindow;
    bool m_reloadWhenEditorFinished;
    
    QPointer<QDialog> m_historyDialog;
    QList<QAction *> m_actions;
    bool m_distUpgradeAvailable;
    QNetworkConfigurationManager* m_config;
    bool m_canExit;

};

#endif // QAPTACTIONS_H
