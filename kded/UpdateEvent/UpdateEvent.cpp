/***************************************************************************
 *   Copyright © 2009 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "UpdateEvent.h"

#include <KDebug>
#include <KProcess>
#include <KToolInvocation>

UpdateEvent::UpdateEvent(QObject* parent, const QString &name)
        : Event(parent, name)
{
}

UpdateEvent::~UpdateEvent()
{
}

bool UpdateEvent::updatesAvailable()
{
    KProcess checkerProcess;
    checkerProcess.setProgram(QStringList() << "/usr/lib/update-notifier/apt-check");

    if (checkerProcess.execute() == 0) {
        return true;
    }
    return false;
}

void UpdateEvent::show()
{
    if (!updatesAvailable()) {
        kDebug() << "No updates available";
        return;
    }

    QPixmap icon = KIcon("system-software-update").pixmap(NOTIFICATION_ICON_SIZE);
    QString text(i18nc("Notification when updates are available",
                       "Updates Available"));
    QStringList actions;
    actions << i18nc("Start the update", "Update");
    actions << i18nc("Button to dismiss this notification once", "Ignore for now");

    Event::show(icon, text, actions);
}

void UpdateEvent::run()
{
    KToolInvocation::kdeinitExec("/usr/bin/muon-updater");

    Event::run();
}

#include "UpdateEvent.moc"
