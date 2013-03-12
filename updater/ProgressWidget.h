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

#ifndef PROGRESSWIDGET_H
#define PROGRESSWIDGET_H

#include <QtGui/QWidget>

class ResourcesUpdatesModel;
class QLabel;
class QParallelAnimationGroup;
class QPushButton;
class QProgressBar;

namespace Ui { class ProgressWidget; }

class ProgressWidget : public QWidget
{
    Q_OBJECT
public:
    ProgressWidget(ResourcesUpdatesModel* updates, QWidget *parent);
    virtual ~ProgressWidget();

private:
    ResourcesUpdatesModel* m_updater;
    qreal m_lastRealProgress;
    bool m_show;

    QParallelAnimationGroup *m_expandWidget;
    Ui::ProgressWidget* m_ui;

public Q_SLOTS:
    void show();
    void animatedHide();

private Q_SLOTS:
    void updateProgress();
    void downloadSpeedChanged();
    void etaChanged();
    void cancelChanged();
    void updateIsProgressing();
};

#endif // PROGRESSWIDGET_H
