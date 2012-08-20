/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef QAPTINTEGRATION_H
#define QAPTINTEGRATION_H

#include <QObject>
#include <LibQApt/Globals>

#include "libmuonprivate_export.h"

class KXmlGuiWindow;
class KActionCollection;
namespace QApt {
class Backend;
}

class KAction;
class KXmlGuiWindow;
class MUONPRIVATE_EXPORT QAptIntegration : public QObject
{
    Q_OBJECT
    friend class MuonMainWindow;
    public:
        explicit QAptIntegration(KXmlGuiWindow* parent = 0);
        void initializationErrors(const QString& errors);

    protected:
        QApt::Backend *m_backend;
        QList<QVariantMap> m_warningStack;
        QList<QVariantMap> m_errorStack;

        QApt::CacheState m_originalState;
        int m_powerInhibitor;
        bool m_canExit;
        bool m_isReloading;
        bool isConnected() const;
        KActionCollection* actionCollection();

    protected Q_SLOTS:
        virtual void initObject();
        virtual void slotQuit();
        virtual bool queryExit();
        virtual void checkForUpdates();
        virtual void workerEvent(QApt::WorkerEvent event);
        virtual void errorOccurred(QApt::ErrorCode code, const QVariantMap &args);
        virtual void warningOccurred(QApt::WarningCode warning, const QVariantMap &args);
        virtual void questionOccurred(QApt::WorkerQuestion question, const QVariantMap &details);
        virtual void showQueuedWarnings();
        virtual void showQueuedErrors();
        virtual void reload();
        void networkChanged();
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
        void easterEggTriggered();
        void setActionsEnabled(bool enabled = true);

    public slots:
        virtual void setupActions();

    Q_SIGNALS:
        void backendReady(QApt::Backend *backend);
        void shouldConnect(bool isConnected);

    private:
        bool m_actionsDisabled;
        KXmlGuiWindow* m_mainWindow;
};

#endif // QAPTINTEGRATION_H
