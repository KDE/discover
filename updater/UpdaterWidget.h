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

#ifndef UPDATERWIDGET_H
#define UPDATERWIDGET_H

#include <QtGui/QWidget>

class QStandardItemModel;
class QTreeView;

namespace QApt {
    class Backend;
}

class Application;
class UpdateModel;

class UpdaterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdaterWidget(QWidget *parent = 0);

private:
    QApt::Backend *m_backend;
    UpdateModel *m_updateModel;
    QList<Application *> m_upgradeableApps;

    QTreeView *m_updateView;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void reload();

private Q_SLOTS:
    void populateUpdateModel();
    void checkApps(QList<Application *> apps, bool checked);
    void checkApp(Application *app, bool checked);
};

#endif // UPDATERWIDGET_H
