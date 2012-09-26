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

#include <LibQApt/Globals>

#include "libmuonprivate_export.h"

class KActionCollection;

namespace QApt {
    class Backend;
}

class MuonMainWindow;

class MUONPRIVATE_EXPORT QAptActions : public QObject
{
    Q_OBJECT
public:
    QAptActions(MuonMainWindow *parent, QApt::Backend *backend);

    bool isConnected() const;
    void setOriginalState(QApt::CacheState state);
    void setCanExit(bool canExit);
    
signals:
    void checkForUpdates();
    void shouldConnect(bool isConnected);
    void changesReverted();
    
public slots:
    void setupActions();
    void setActionsEnabled(bool enabled = true);
    void networkChanged();

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
    void runSourcesEditor(bool update = false);
    void sourcesEditorFinished(int reload);

private:
    QApt::Backend *m_backend;
    QApt::CacheState m_originalState;
    bool m_actionsDisabled;
    MuonMainWindow* m_mainWindow;

    KActionCollection* actionCollection();
};

#endif // QAPTACTIONS_H
