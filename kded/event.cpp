/***************************************************************************
 *   Copyright © 2009 Harald Sitter <apachelogger@ubuntu.com>              *
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

#include "event.h"

#include <QtCore/QStringBuilder>

#include <KActionCollection>
#include <KConfigGroup>
#include <KMenu>
#include <KNotification>
#include <KStatusNotifierItem>

Event::Event(QObject* parent, const QString &name)
        : QObject(parent)
        , m_name(name)
        , m_hidden(false)
        , m_active(false)
{
    m_hiddenCfgString = QString("hide" % m_name % "Notifier");
    m_hidden = readHiddenConfig();
    readNotifyConfig();
}

Event::~Event()
{
}

bool Event::readHiddenConfig()
{
    KConfig cfg("muon-notifier");
    KConfigGroup notifyGroup(&cfg, "Event");
    return notifyGroup.readEntry(m_hiddenCfgString, false);
}

void Event::writeHiddenConfig(bool value)
{
    KConfig cfg("muon-notifier");
    KConfigGroup notifyGroup(&cfg, "Event");
    notifyGroup.writeEntry(m_hiddenCfgString, value);
    notifyGroup.config()->sync();
}

void Event::readNotifyConfig()
{
    KConfig cfg("muon-notifier");
    KConfigGroup notifyTypeGroup(&cfg, "NotificationType");
    QString notifyType = notifyTypeGroup.readEntry("NotifyType", "Combo");

    if (notifyType == "Combo") {
        m_useKNotify = true;
        m_useTrayIcon = true;
    } else if (notifyType == "TrayOnly") {
        m_useKNotify = false;
        m_useTrayIcon = true;
    } else { // KNotifyOnly
        m_useKNotify = true;
        m_useTrayIcon = false;
    }
}

bool Event::isHidden() const
{
    return m_hidden;
}

void Event::show(const KIcon &icon, const QString &text, const QStringList &actions)
{
    if (m_active || m_hidden) {
        return;
    }

    if (m_useKNotify) {
        KNotification::NotificationFlag flag;

        if (!m_useTrayIcon) {
            // Tray icon not in use, so be persistant
            flag = KNotification::Persistant;
        }

        m_active = true;
        KNotification *notify = new KNotification(m_name, 0, flag);
        notify->setComponentData(KComponentData("muon-notifier"));

        notify->setPixmap(icon.pixmap(NOTIFICATION_ICON_SIZE));
        notify->setText(text);
        notify->setActions(actions);

        if (!m_useTrayIcon) {
            // Tray icon not in use to handle actions
            connect(notify, SIGNAL(action1Activated()), this, SLOT(run()));
            connect(notify, SIGNAL(action2Activated()), this, SLOT(ignore()));
            connect(notify, SIGNAL(action3Activated()), this, SLOT(hide()));
        }

        connect(notify, SIGNAL(closed()), this, SLOT(notifyClosed()));

        notify->sendEvent();
    }

    if (m_useTrayIcon) {
        m_notifierItem = new KStatusNotifierItem(this);
        m_notifierItem->setIconByPixmap(icon);
        m_notifierItem->setToolTipIconByPixmap(icon);
        m_notifierItem->setToolTipTitle(i18n("System Notification Helper"));
        m_notifierItem->setToolTipSubTitle(text);
        m_notifierItem->setStatus(KStatusNotifierItem::NeedsAttention);
        m_notifierItem->setCategory(KStatusNotifierItem::SystemServices);
        m_notifierItem->setStandardActionsEnabled(false);

        KMenu *contextMenu = new KMenu(0);
        contextMenu->addTitle(KIcon("applications-system"), i18n("System Notification Helper"));

        QAction *runAction = contextMenu->addAction(actions.at(0));
        runAction->setIcon(icon);
        connect(runAction, SIGNAL(triggered()), this, SLOT(run()));
        contextMenu->addAction(runAction);

        QAction *ignoreForeverAction = contextMenu->addAction(actions.at(2));
        connect(ignoreForeverAction, SIGNAL(triggered()), this, SLOT(hide()));
        contextMenu->addAction(ignoreForeverAction);

        contextMenu->addSeparator();

        QAction *hideAction = contextMenu->addAction(i18n("Hide"));
        hideAction->setIcon(KIcon("application-exit"));
        connect(hideAction, SIGNAL(triggered()), this, SLOT(ignore()));
        contextMenu->addAction(hideAction);

        m_notifierItem->setContextMenu(contextMenu);
        m_notifierItem->setAssociatedWidget(NULL);

        connect(m_notifierItem, SIGNAL(activateRequested(bool, const QPoint &)), this, SLOT(run()));
    }
}

void Event::run()
{
    delete m_notifierItem;
    m_notifierItem = 0;
    notifyClosed();
}

void Event::ignore()
{
    m_notifierItem->deleteLater();
    m_notifierItem = 0;
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
