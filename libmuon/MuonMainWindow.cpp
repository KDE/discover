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

#include "MuonMainWindow.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QShortcut>

// KDE includes
#include <KActionCollection>
#include <KApplication>
#include <KDialog>
#include <KStandardAction>
#include <KStandardDirs>
#include <KVBox>
#include <Phonon/MediaObject>

#ifdef QAPT_ENABLED
#include <QAptActions.h>
#endif

MuonMainWindow::MuonMainWindow()
    : KXmlGuiWindow(0)
    , m_canExit(false)
#ifdef QAPT_ENABLED
    , m_actions(nullptr)
#endif
{
}

bool MuonMainWindow::queryExit()
{
    return m_canExit;
}

QSize MuonMainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(900, 500));
}

void MuonMainWindow::setupActions()
{
    KAction *quitAction = KStandardAction::quit(KApplication::instance(),
                                                SLOT(quit()), actionCollection());
    actionCollection()->addAction("quit", quitAction);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(easterEggTriggered()));
#ifdef QAPT_ENABLED
    m_actions->setupActions();
#endif
}

void MuonMainWindow::easterEggTriggered()
{
    KDialog *dialog = new KDialog(this);
    KVBox *widget = new KVBox(dialog);
    QLabel *label = new QLabel(widget);
    label->setText(i18nc("@label Easter Egg", "This Muon has super cow powers"));
    QLabel *moo = new QLabel(widget);
    moo->setFont(QFont("monospace"));
    moo->setText("             (__)\n"
                 "             (oo)\n"
                 "    /---------\\/\n"
                 "   / | Muuu!!||\n"
                 "  *  ||------||\n"
                 "     ^^      ^^\n");

    dialog->setMainWidget(widget);
    dialog->show();

    QString mooFile = KStandardDirs::locate("data", "libmuon/moo.ogg");
    Phonon::MediaObject *music =
        Phonon::createPlayer(Phonon::MusicCategory,
                             Phonon::MediaSource(mooFile));
    music->play();
}

void MuonMainWindow::setCanExit(bool canExit)
{
    m_canExit = canExit;
}

void MuonMainWindow::setActionsEnabled(bool enabled)
{
#ifdef QAPT_ENABLED
    m_actions->setActionsEnabled(enabled);
#endif
}