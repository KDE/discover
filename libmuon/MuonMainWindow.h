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

#include "libmuonprivate_export.h"

class KAction;
class QAptIntegration;

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
class MUONPRIVATE_EXPORT MuonMainWindow : public KXmlGuiWindow
{
    Q_OBJECT
    public:
        MuonMainWindow();
        virtual ~MuonMainWindow();

        QSize sizeHint() const;
        void setupActions();
        void initializationErrors(const QString& errors);
        bool isConnected();
        virtual void revertChanges();
        void setActionsEnabled(bool enabled = true);
        
    Q_SIGNALS:
        void backendReady(QApt::Backend*);
        void shouldConnect(bool isConnected);

    protected slots:
        void initObject();
        virtual void workerEvent(QApt::WorkerEvent event);
        virtual void errorOccurred(QApt::ErrorCode code, const QVariantMap &args);
        virtual void warningOccurred(QApt::WarningCode warning, const QVariantMap &args);
        virtual void questionOccurred(QApt::WorkerQuestion question, const QVariantMap &details);
        virtual void showQueuedWarnings();
        virtual void showQueuedErrors();
        void downloadPackagesFromList();
        bool saveSelections();
        bool saveInstalledPackagesList();
        void loadSelections();
        bool createDownloadList();
        void loadArchives();

    public slots:
        void runSourcesEditor(bool update = false);
        void sourcesEditorFinished(int reload);
        void easterEggTriggered();

    protected:
        QAptIntegration* m_aptify;
        QApt::Backend*& m_backend;
        bool& m_canExit;
        bool& m_isReloading;
        QApt::CacheState& m_originalState;
        QList<QVariantMap>& m_warningStack;
        QList<QVariantMap>& m_errorStack;
};

#endif
