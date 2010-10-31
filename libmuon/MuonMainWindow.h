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

#ifndef MUONMAINWINDOW_H
#define MUONMAINWINDOW_H

// Qt includes
#include <QtCore/QVariantMap>

// KDE includes
#include <KXmlGuiWindow>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Globals>

class KAction;

namespace QApt
{
    class Backend;
}

/**
 * This class serves as a shared Main Window implementation that connects
 * all the various backend bits so that they don't have to be reimplemented
 * in things like an update-centric GUI, etc.
 *
 * @short Main window class
 * @author Jonathan Thomas <echidnaman@kubuntu.org>
 * @version 0.1
 */
class MuonMainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MuonMainWindow();
    virtual ~MuonMainWindow();

    bool isConnected();

protected:
    QApt::Backend *m_backend;
    QList<QVariantMap> m_warningStack;
    QList<QVariantMap> m_errorStack;

    KAction *m_updateAction;
    KAction *m_undoAction;
    KAction *m_redoAction;
    KAction *m_revertAction;

    int m_powerInhibitor;
    bool m_canExit;

protected Q_SLOTS:
    virtual void initObject();
    virtual void slotQuit();
    virtual bool queryExit();
    virtual void setupActions();
    virtual void workerEvent(QApt::WorkerEvent event);
    virtual void errorOccurred(QApt::ErrorCode code, const QVariantMap &args);
    virtual void warningOccurred(QApt::WarningCode warning, const QVariantMap &args);
    virtual void questionOccurred(QApt::WorkerQuestion question, const QVariantMap &details);
    virtual void showQueuedWarnings();
    virtual void showQueuedErrors();
    virtual void reload();
    void setActionsEnabled(bool enabled = true);
    void networkChanged();
    bool saveSelections();
    bool saveInstalledPackagesList();
    void loadSelections();
    void undo();
    void redo();
    virtual void revertChanges();

Q_SIGNALS:
    void backendReady(QApt::Backend *backend);
    void shouldConnect(bool isConnected);

private:
    bool m_actionsDisabled;
};

#endif
