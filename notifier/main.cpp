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
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include "DiscoverNotifier.h"
#include "../DiscoverVersion.h"

#include "NotifierItem.h"
#include "../DiscoverVersion.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    KCrash::setFlags(KCrash::AutoRestart);

    NotifierItem notifier;
    bool hide = false;
    {
        KAboutData about(QStringLiteral("DiscoverNotifier"), i18n("Discover Notifier"), version, i18n("System update status notifier"),
                     KAboutLicense::GPL, i18n("© 2010-2019 Plasma Development Team"));
        about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
        about.setProductName("discover/discover");
        about.setProgramLogo(app.windowIcon());
        about.setTranslator(
                i18ndc(nullptr, "NAME OF TRANSLATORS", "Your names"),
                i18ndc(nullptr, "EMAIL OF TRANSLATORS", "Your emails"));

        QCommandLineParser parser;
        QCommandLineOption replaceOption({QStringLiteral("replace")},
                                 i18n("Replace an existing instance"));
        parser.addOption(replaceOption);
        QCommandLineOption hideOption({QStringLiteral("hide")}, i18n("Do not show the notifier"), i18n("hidden"), QStringLiteral("false"));
        parser.addOption(hideOption);
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

        const auto config = KSharedConfig::openConfig();
        KConfigGroup group(config, "Behavior");

        if (parser.isSet(hideOption)) {
            hide = parser.value(hideOption) == QLatin1String("true");
            group.writeEntry<bool>("Hide", hide);
            config->sync();
        } else {
            hide = group.readEntry<bool>("Hide", false);
        }
    }

    KDBusService service(KDBusService::Unique);
    notifier.setVisible(!hide);

    return app.exec();
}
