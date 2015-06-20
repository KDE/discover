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
#include <QLabel>
#include <QShortcut>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QMenu>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QNetworkConfigurationManager>

// KF5 includes
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <phonon/MediaObject>

MuonMainWindow::MuonMainWindow()
    : KXmlGuiWindow(0)
    , m_canExit(true)
{
}

bool MuonMainWindow::queryClose()
{
    return m_canExit;
}

QSize MuonMainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(900, 500));
}

void MuonMainWindow::setupActions()
{
    QAction *quitAction = KStandardAction::quit(QApplication::instance(),
                                                SLOT(quit()), actionCollection());
    actionCollection()->addAction("file_quit", quitAction);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(easterEggTriggered()));
}

void MuonMainWindow::easterEggTriggered()
{
    QDialog *dialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QLabel *label = new QLabel(dialog);
    label->setText(i18nc("@label Easter Egg", "This Muon has super cow powers"));
    QLabel *moo = new QLabel(dialog);
    moo->setFont(QFont("monospace"));
    moo->setText("             (__)\n"
                 "             (oo)\n"
                 "    /---------\\/\n"
                 "   / | Muuu!!||\n"
                 "  *  ||------||\n"
                 "     ^^      ^^\n");

    layout->addWidget(label);
    layout->addWidget(moo);
    dialog->setLayout(layout);
    dialog->show();

    QUrl mooFile = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "libmuon/moo.ogg"));
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
    for (int i = 0; i < actionCollection()->count(); ++i) {
        QAction* a=actionCollection()->action(i);
        //FIXME: Better solution? (en/dis)abling all actions at once could be dangerous...
        if(QByteArray(a->metaObject()->className())!="DiscoverAction")
            a->setEnabled(enabled);
    }
    if(enabled)
        emit actionsEnabledChanged(enabled);
}
