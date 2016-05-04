/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include <KXmlGuiWindow>
#include <QPointer>

class KMessageWidget;
class ResourcesUpdatesModel;
class ProgressWidget;
class UpdaterWidget;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow();

    QSize sizeHint() const override;
    bool queryClose() override;

private:
    ResourcesUpdatesModel* m_updater;

    ProgressWidget *m_progressWidget;
    UpdaterWidget *m_updaterWidget;
    KMessageWidget *m_powerMessage;
    QAction *m_applyAction;
    QMenu* m_moreMenu;
    QMenu* m_advancedMenu;
    QWidget* m_controls;

private Q_SLOTS:
    void setActionsEnabled() { setActionsEnabled(true); }
    void setActionsEnabled(bool enabled);

    void initGUI();
    void initBackend();
    void setupActions();
    void setupBackendsActions();
    void checkPlugState();
    void updatePlugState(bool plugged);
    void progressingChanged();
};

#endif // MAINWINDOW_H
