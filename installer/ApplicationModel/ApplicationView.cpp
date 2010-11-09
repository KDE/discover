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

#include "ApplicationView.h"

#include <QtCore/QDir>

#include <KDebug>

#include <LibQApt/Backend>

#include "ApplicationModel.h"
#include "ApplicationProxyModel.h"
#include "ApplicationDelegate.h"

ApplicationView::ApplicationView(QWidget *parent)
        : QTreeView(parent)
{
    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setUniformRowHeights(true);

    m_appModel = new ApplicationModel(this);
    m_proxyModel = new ApplicationProxyModel(this);
    m_proxyModel->setSourceModel(m_appModel);
    ApplicationDelegate *delegate = new ApplicationDelegate(this);

    setModel(m_proxyModel);
    setItemDelegate(delegate);
}

ApplicationView::~ApplicationView()
{
}

void ApplicationView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    setSortingEnabled(true);
    m_proxyModel->setBackend(backend);
    reload();
    sortByColumn(0, Qt::AscendingOrder);
}

void ApplicationView::reload()
{
    m_appModel->clear();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);

    QList<Application*> list;
    QList<int> popconScores;
    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QDir::Files);
    foreach(const QString &fileName, fileList) {
        Application *app = new Application("/usr/share/app-install/desktop/" + fileName, m_backend);
        if (app->isValid()) {
            list << app;
            popconScores << app->popconScore();
        } else {
            // Invalid .desktop file
            kDebug() << fileName;
        }
    }
    qSort(popconScores);
    kDebug() << popconScores.last();
    m_appModel->setMaxPopcon(popconScores.last());
    m_appModel->setApplications(list);

    m_proxyModel->setSourceModel(m_appModel);
}

void ApplicationView::setStateFilter(QApt::Package::State state)
{
    m_proxyModel->setStateFilter(state);
}

void ApplicationView::setOriginFilter(const QString &origin)
{
    m_proxyModel->setOriginFilter(origin);
}

#include "ApplicationView.moc"
