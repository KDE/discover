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

#include "ApplicationWindow.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>

// KDE includes
#include <KIcon>
#include <KDebug>

// Own includes
#include "CategoryView.h"
#include "OriginView.h"
#include "ApplicationModel/ApplicationView.h"

ApplicationWindow::ApplicationWindow()
    : MuonMainWindow()
    , m_powerInhibitor(0)
{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

ApplicationWindow::~ApplicationWindow()
{
}

void ApplicationWindow::initGUI()
{
    setWindowTitle(i18n("Muon Software Center"));

    m_mainWidget = new QSplitter(this);
    m_mainWidget->setOrientation(Qt::Horizontal);
    connect(m_mainWidget, SIGNAL(splitterMoved(int, int)), this, SLOT(saveSplitterSizes()));
    setCentralWidget(m_mainWidget);

    m_mainView = new QStackedWidget(this);
    m_mainWidget->addWidget(m_mainView);

    OriginView *originView = new OriginView(this);
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            originView, SLOT(setBackend(QApt::Backend *)));
    connect(originView, SIGNAL(activated(const QModelIndex &)),
           this, SLOT(changeView(const QModelIndex &)));
    m_mainWidget->addWidget(originView);

    m_appView = new ApplicationView(this);
    m_mainWidget->addWidget(m_appView);

    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            m_appView, SLOT(setBackend(QApt::Backend *)));
    connect(this, SIGNAL(backendReady(QApt::Backend *)),
            this, SLOT(reload()));
}

void ApplicationWindow::reload()
{
}

void ApplicationWindow::changeView(const QModelIndex &index)
{
    kDebug() << index;
}

#include "ApplicationWindow.moc"
