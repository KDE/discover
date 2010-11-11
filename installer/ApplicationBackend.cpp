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

#include "ApplicationBackend.h"

#include <QtCore/QDir>
#include <QtCore/QStringList>

#include <LibQApt/Backend>

#include "Application.h"

ApplicationBackend::ApplicationBackend(QObject *parent)
    : QObject(parent)
    , m_backend(0)
{
}

ApplicationBackend::~ApplicationBackend()
{
}

void ApplicationBackend::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    reload();
}

void ApplicationBackend::reload()
{
    qDeleteAll(m_appList);
    QList<int> popconScores;
    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QDir::Files);
    foreach(const QString &fileName, fileList) {
        Application *app = new Application("/usr/share/app-install/desktop/" + fileName, m_backend);
        if (app->isValid()) {
            m_appList << app;
            popconScores << app->popconScore();
        } else {
            // Invalid .desktop file
            // kDebug() << fileName;
        }
    }
    qSort(popconScores);

    m_maxPopconScore = popconScores.last();
}

QList<Application *> ApplicationBackend::applicationList() const
{
    return m_appList;
}

int ApplicationBackend::maxPopconScore() const
{
    return m_maxPopconScore;
}
