/*
 *   Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QApplication>
#include <KStatusNotifierItem>
#include <QMenu>
#include <KLocalizedString>
#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include "DiscoverNotifier.h"
#include "../DiscoverVersion.h"

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

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    KCrash::setFlags(KCrash::AutoRestart);

    {
        KAboutData about(QStringLiteral("DiscoverNotifier"), i18n("Discover Notifier"), version, i18n("System update status notifier"),
                     KAboutLicense::GPL, i18n("© 2010-2019 Plasma Development Team"));
        about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
        about.setProductName("discover/discover");
        about.setProgramLogo(app.windowIcon());

        QCommandLineParser parser;
        QCommandLineOption replaceOption({QStringLiteral("replace")},
                                 i18n("Replace an existing instance"));
        parser.addOption(replaceOption);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);

        if (parser.isSet(replaceOption)) {
            auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.DiscoverNotifier"),
                                                        QStringLiteral("/MainApplication"),
                                                        QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                        QStringLiteral("quit"));
            auto reply = QDBusConnection::sessionBus().call(message); //deliberately block until it's done, so we register the name after the app quits

            while (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.DiscoverNotifier"))) {
                QCoreApplication::processEvents(QEventLoop::AllEvents);
            }
        }
    }

    KDBusService service(KDBusService::Unique);
    DiscoverNotifier notifier;

    KStatusNotifierItem item;
    item.setTitle(i18n("Updates"));
    item.setToolTipTitle(i18n("Updates"));

    auto refresh = [&notifier, &item](){
        item.setStatus(sniStatus(notifier.state()));
        item.setIconByName(notifier.iconName());
        item.setToolTipSubTitle(notifier.message());
    };

    QObject::connect(&notifier, &DiscoverNotifier::stateChanged, &item, refresh);

    QObject::connect(&item, &KStatusNotifierItem::activateRequested, &notifier, [&notifier]() {
        notifier.showDiscoverUpdates();
    });

    QMenu* menu = new QMenu;
    auto discoverAction = menu->addAction(QIcon::fromTheme(QStringLiteral("plasma-discover")), i18n("Open Software Center..."));
    QObject::connect(discoverAction, &QAction::triggered, &notifier, &DiscoverNotifier::showDiscover);

    auto updatesAction = menu->addAction(QIcon::fromTheme(QStringLiteral("system-software-update")), i18n("See Updates..."));
    QObject::connect(updatesAction, &QAction::triggered, &notifier, &DiscoverNotifier::showDiscoverUpdates);

    auto refreshAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Refresh..."));
    QObject::connect(refreshAction, &QAction::triggered, &notifier, &DiscoverNotifier::recheckSystemUpdateNeeded);

    auto f = [menu, &notifier]() {
        auto refreshAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Restart..."));
        QObject::connect(refreshAction, &QAction::triggered, &notifier, &DiscoverNotifier::recheckSystemUpdateNeeded);
    };
    if (notifier.needsReboot())
        f();
    else
        QObject::connect(&notifier, &DiscoverNotifier::needsRebootChanged, menu, f);

    QObject::connect(&notifier, &DiscoverNotifier::newUpgradeAction, menu, [menu](UpgradeAction* a) {
        QAction* action = new QAction(a->description(), menu);
        QObject::connect(action, &QAction::triggered, a, &UpgradeAction::trigger);
        menu->addAction(action);
    });
    item.setContextMenu(menu);
    refresh();

    return app.exec();
}
