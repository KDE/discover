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
#include "ApplicationDelegate.h"

ApplicationView::ApplicationView(QWidget *parent)
        : QTreeView(parent)
{
    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setUniformRowHeights(true);

    m_appModel = new ApplicationModel(this);
    ApplicationDelegate *delegate = new ApplicationDelegate(this);

    setModel(m_appModel);
    setItemDelegate(delegate);
}

ApplicationView::~ApplicationView()
{
}

void ApplicationView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    reload();
}

void ApplicationView::reload()
{
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
}

#include "ApplicationView.moc"
