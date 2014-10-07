/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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

#include "NotifySettingsPage.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <qdbusinterface.h>
#include <qdbusreply.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QDialog>

#include <KConfig>
#include <KConfigGroup>
#include <klocalizedstring.h>
#include <kservicetypetrader.h>

NotifySettingsPage::NotifySettingsPage(QWidget* parent) :
        SettingsPageBase(parent)
{
    KService::List offers = KServiceTypeTrader::self()->query( "KDEDModule" );
    KService::List::const_iterator end = offers.constEnd();
    for (KService::List::const_iterator it = offers.constBegin(); it != end; ++it) {
        if ((*it)->desktopEntryName().startsWith("muon")) {
            m_services << (*it)->desktopEntryName();
        }
    }

    m_kded = new QDBusInterface("org.kde.kded", "/kded",
                                "org.kde.kded", QDBusConnection::sessionBus(), this);
    QDBusReply<QStringList> lM = m_kded->call("loadedModules");
    QStringList loadedModules = lM.value();
    foreach (const QString &module, loadedModules) {
        if (m_services.contains(module))
            m_loadedModules << module;
    }
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);

    m_updatesCheckBox = new QCheckBox(i18n("Show notifications for available updates"), this);
    m_verboseCheckBox = new QCheckBox(i18n("Show the number of available updates"), this);

    connect(m_updatesCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_verboseCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_updatesCheckBox, SIGNAL(clicked(bool)), m_verboseCheckBox, SLOT(setEnabled(bool)));
    
    QSpacerItem * vSpacer = new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);

    layout->addWidget(m_updatesCheckBox);
    layout->addWidget(m_verboseCheckBox);
    layout->addSpacerItem(vSpacer);

    loadSettings();
}

NotifySettingsPage::~NotifySettingsPage()
{
}

void NotifySettingsPage::loadSettings()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);

    m_updatesCheckBox->setChecked(!m_loadedModules.isEmpty());

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    bool verbose = notifyTypeGroup.readEntry("Verbose", false);

    m_verboseCheckBox->setChecked(verbose);
    m_verboseCheckBox->setEnabled(m_updatesCheckBox->isChecked());
}

void NotifySettingsPage::applySettings()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");

    notifyTypeGroup.writeEntry("Verbose", m_verboseCheckBox->isChecked());
    notifyTypeGroup.sync();

    if (m_updatesCheckBox->isChecked()) {
        foreach (const QString &service, m_services) {
            if (!m_loadedModules.contains(service)) {
                m_kded->call("loadModule", service);
                m_kded->call("setModuleAutoloading", service, true);
            }
            QDBusMessage message = QDBusMessage::createMethodCall("org.kde.kded",
                                    "/modules/" + service,
                                    "org.kde.kded.AbstractKDEDModule",
                                    "configurationChanged");
            QDBusConnection::sessionBus().send(message);
        }
    } else {
        foreach (const QString &service, m_services) {
            m_kded->call("setModuleAutoloading", service, false);
            m_kded->call("unloadModule", service);
        }
    }
}

void NotifySettingsPage::restoreDefaults()
{
}

#include "NotifySettingsPage.moc"
