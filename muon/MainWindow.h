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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Own includes
#include <KXmlGuiWindow>
#include <QApt/Globals>

class QSplitter;
class QStackedWidget;
class QToolBox;
class KDialog;

class ManagerSettingsDialog;
class FilterWidget;
class ManagerWidget;
class ReviewWidget;
class TransactionWidget;
class StatusWidget;

namespace QApt {
    class Backend;
    class Transaction;
}

/**
 * This class serves as the main window for Muon.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Jonathan Thomas <echidnaman@kubuntu.org>
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

    QSize sizeHint() const Q_DECL_OVERRIDE;
    bool queryClose() Q_DECL_OVERRIDE;

private:
    QApt::Backend *m_backend;
    QApt::Transaction *m_trans;

    QStackedWidget *m_stack;
    QSplitter *m_mainWidget;
    QAction *m_safeUpgradeAction;
    QAction *m_distUpgradeAction;
    QAction *m_autoRemoveAction;
    QAction *m_previewAction;
    QAction *m_applyAction;
    QAction *m_saveInstalledAction;
    QAction *m_saveSelectionsAction;
    QAction *m_loadSelectionsAction;
    QAction *m_createDownloadListAction;
    QAction *m_downloadListAction;
    QAction *m_loadArchivesAction;

    ManagerSettingsDialog *m_settingsDialog;
    KDialog *m_historyDialog;
    FilterWidget *m_filterBox;
    ManagerWidget *m_managerWidget;
    ReviewWidget *m_reviewWidget;
    TransactionWidget *m_transWidget;
    StatusWidget *m_statusWidget;
    bool m_reloading;

private Q_SLOTS:
    void initGUI();
    void initObject();
    void loadSplitterSizes();
    void loadSettings();
    void saveSplitterSizes();
    void setupActions();
    void markUpgrade();
    void markDistUpgrade();
    void markAutoRemove();
    void checkForUpdates();
    void transactionStatusChanged(QApt::TransactionStatus status);
    void errorOccurred(QApt::ErrorCode error);
    void previewChanges();
    void returnFromPreview();
    void startCommit();
    void reload();
    void setActionsEnabled(bool enabled = true);
    void downloadArchives(QApt::Transaction *trans);

public Q_SLOTS:
    void revertChanges();
    void editSettings();
    void closeSettingsDialog();

    void setupTransaction(QApt::Transaction *trans);

signals:
    void backendReady(QApt::Backend *backend);
};

#endif
