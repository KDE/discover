/***************************************************************************
 *   Copyright © 2009 Harald Sitter <apachelogger@ubuntu.com>              *
 *   Copyright © 2009-2010 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "event.h"

#include <QtCore/QStringBuilder>

#include <KConfigGroup>
#include <KNotification>

Event::Event(QObject* parent, const QString &name)
        : QObject(parent)
        , m_name(name)
        , m_hidden(false)
        , m_active(false)
{
    m_cfgstring = QString("hide" % m_name % "Notifier");
    m_hidden = readHiddenConfig();
}

Event::~Event()
{
}

bool Event::readHiddenConfig()
{
    KConfig cfg("muon-notifier");
    KConfigGroup notifyGroup(&cfg, "Event");
    return notifyGroup.readEntry(m_cfgstring, false);
}

void Event::writeHiddenConfig(bool value)
{
    KConfig cfg("muon-notifier");
    KConfigGroup notifyGroup(&cfg, "Event");
    notifyGroup.writeEntry(m_cfgstring, value);
    notifyGroup.config()->sync();
}

bool Event::isHidden() const
{
    return m_hidden;
}

void Event::show(const QPixmap &icon, const QString &text, const QStringList &actions)
{
    if (m_active || m_hidden) {
        return;
    }

    m_active = true;
    KNotification *notify = new KNotification(m_name, 0, KNotification::Persistent);
    notify->setComponentData(KComponentData("notificationhelper"));

    notify->setPixmap(icon);
    notify->setText(text);
    notify->setActions(actions);

    connect(notify, SIGNAL(action1Activated()), this, SLOT(run()));
    connect(notify, SIGNAL(action2Activated()), this, SLOT(ignore()));
    connect(notify, SIGNAL(action3Activated()), this, SLOT(hide()));

    connect(notify, SIGNAL(closed()), this, SLOT(notifyClosed()));

    notify->sendEvent();
}

void Event::run()
{
    notifyClosed();
}

void Event::ignore()
{
    notifyClosed();
}

void Event::hide()
{
    notifyClosed();
    writeHiddenConfig(true);
    m_hidden = true;
}

void Event::notifyClosed()
{
    m_active = false;
}

void Event::reloadConfig()
{
    m_hidden = readHiddenConfig();
}

#include "event.moc"
