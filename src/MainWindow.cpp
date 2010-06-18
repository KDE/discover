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

#include "MainWindow.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBox>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KConfigDialog>
#include <KLocale>
#include <KStandardAction>
#include <KStatusBar>
#include <KDebug>

#include <libqapt/backend.h>

#include "FilterWidget.h"
#include "ManagerWidget.h"

MainWindow::MainWindow()
    : KXmlGuiWindow(),
      m_backend(0),
      m_stack(0)

{
    m_backend = new QApt::Backend;
    m_backend->init();

    m_stack = new QStackedWidget;
    setCentralWidget(m_stack);

    m_filterBox = new FilterWidget(m_stack);
    m_managerWidget = new ManagerWidget(m_stack, m_backend);

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(m_filterBox);
    splitter->addWidget(m_managerWidget);

    m_stack->addWidget(splitter);
    m_stack->setCurrentWidget(splitter);

    setupActions();

//     QLabel* packageCountLabel = new QLabel(this);
//     packageCountLabel->setText(i18np("%1 item available", "%1 items available", m_backend->packageCount()));
//     statusBar()->addWidget(packageCountLabel);
    statusBar()->hide();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupActions()
{
    // local - Destroys all sub-windows and exits
    KAction *quitAction = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);
    setupGUI();
}

void MainWindow::slotQuit()
{
    //Settings::self()->writeConfig();
    KApplication::instance()->quit();
}

#include "MainWindow.moc"
