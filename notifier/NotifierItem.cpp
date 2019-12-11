/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "NotifierItem.h"
#include <KStatusNotifierItem>
#include <KLocalizedString>
#include <QMenu>
#include <QDebug>

KStatusNotifierItem::ItemStatus sniStatus(DiscoverNotifier::State state)
{
    switch (state) {
        case DiscoverNotifier::Offline:
        case DiscoverNotifier::NoUpdates:
            return KStatusNotifierItem::Passive;
        case DiscoverNotifier::NormalUpdates:
        case DiscoverNotifier::SecurityUpdates:
        case DiscoverNotifier::RebootRequired:
            return KStatusNotifierItem::Active;
    }
    return KStatusNotifierItem::Active;
}


NotifierItem::NotifierItem()
{
}

void NotifierItem::setupNotifierItem()
{
    Q_ASSERT(!m_item);
    m_item = new KStatusNotifierItem(QStringLiteral("org.kde.DiscoverNotifier"), this);
    m_item->setTitle(i18n("Updates"));
    m_item->setToolTipTitle(i18n("Updates"));

    connect(&m_notifier, &DiscoverNotifier::stateChanged, this, &NotifierItem::refresh);

    connect(m_item, &KStatusNotifierItem::activateRequested, &m_notifier, [this]() {
        m_notifier.showDiscoverUpdates();
    });

    QMenu* menu = new QMenu;
    connect(m_item, &QObject::destroyed, menu, &QObject::deleteLater);
    auto discoverAction = menu->addAction(QIcon::fromTheme(QStringLiteral("plasma-discover")), i18n("Open Discover..."));
    connect(discoverAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::showDiscover);

    auto updatesAction = menu->addAction(QIcon::fromTheme(QStringLiteral("system-software-update")), i18n("See Updates..."));
    connect(updatesAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::showDiscoverUpdates);

    auto refreshAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Refresh..."));
    connect(refreshAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::recheckSystemUpdateNeeded);

    auto f = [menu, this]() {
        auto refreshAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Restart..."));
        connect(refreshAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::recheckSystemUpdateNeeded);
    };
    if (m_notifier.needsReboot())
        f();
    else
        connect(&m_notifier, &DiscoverNotifier::needsRebootChanged, menu, f);

    connect(&m_notifier, &DiscoverNotifier::newUpgradeAction, menu, [menu](UpgradeAction* a) {
        QAction* action = new QAction(a->description(), menu);
        connect(action, &QAction::triggered, a, &UpgradeAction::trigger);
        menu->addAction(action);
    });
    m_item->setContextMenu(menu);
    refresh();
}

void NotifierItem::refresh()
{
    Q_ASSERT(m_item);
    m_item->setStatus(sniStatus(m_notifier.state()));
    m_item->setIconByName(m_notifier.iconName());
    m_item->setToolTipSubTitle(m_notifier.message());
}

void NotifierItem::setVisible(bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;

    if (m_visible)
        setupNotifierItem();
    else
        delete m_item;
}
