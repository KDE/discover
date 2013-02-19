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

#include <QtGui/QStackedWidget>

class ResourcesUpdatesModel;
class KMessageWidget;
class AbstractResource;
class AbstractResourcesBackend;
class QItemSelection;
class QLabel;
class QStandardItemModel;
class QTreeView;

class KPixmapSequenceOverlayPainter;
class UpdateModel;

class UpdaterWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit UpdaterWidget(QWidget *parent = 0);

private:
    UpdateModel *m_updateModel;

    QTreeView *m_updateView;
    KPixmapSequenceOverlayPainter *m_busyWidget;
    QLabel *m_updateStatusIcon;
    QLabel *m_notifyTitle;
    QLabel *m_notifyDesc;
    ResourcesUpdatesModel* m_updatesBackends;
    KMessageWidget* m_upgradesWidget;

public Q_SLOTS:
    void setBackend(ResourcesUpdatesModel* backend);
    void activityChanged();

private Q_SLOTS:
    void populateUpdateModel();
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);
    void checkAllMarked();
    void checkUpToDate();
    void markAllPackagesForUpgrade();
    void checkApps(const QList<AbstractResource*>& apps, bool checked);

signals:
    void selectedResourceChanged(AbstractResource* res);
};

#endif // UPDATERWIDGET_H
