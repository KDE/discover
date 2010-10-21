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

#include "NotifySettingsPage.h"

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QVBoxLayout>

#include <KConfig>
#include <KDialog>
#include <KLocale>

NotifySettingsPage::NotifySettingsPage(QWidget* parent) :
        SettingsPageBase(parent)
        , m_comboRadio(new QRadioButton(this))
        , m_trayOnlyRadio(new QRadioButton(this))
        , m_KNotifyOnlyRadio(new QRadioButton(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());

    QLabel *label = new QLabel(this);
    label->setText(i18n("Show notifications for:"));

    m_updatesCheckBox = new QCheckBox(i18n("Available updates"), this);
    m_distUpgradeCheckBox = new QCheckBox(i18n("Distribution upgrades"), this);

    connect(m_updatesCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_distUpgradeCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QLabel *label2 = new QLabel(this);
    label2->setText(i18n("Notification type:"));

    QButtonGroup *notifyTypeGroup = new QButtonGroup(this);
    m_comboRadio->setText(i18n("Use both popups and tray icons"));
    m_trayOnlyRadio->setText(i18n("Tray icons only"));
    m_KNotifyOnlyRadio->setText(i18n("Popup notifications only"));

    notifyTypeGroup->addButton(m_comboRadio);
    notifyTypeGroup->addButton(m_trayOnlyRadio);
    notifyTypeGroup->addButton(m_KNotifyOnlyRadio);

    connect(m_comboRadio, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_trayOnlyRadio, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_KNotifyOnlyRadio, SIGNAL(clicked()), this, SIGNAL(changed()));

    layout->addWidget(label);
    layout->addWidget(m_updatesCheckBox);
    layout->addWidget(m_distUpgradeCheckBox);
    layout->addWidget(label2);
    layout->addWidget(spacer);
    layout->addWidget(m_comboRadio);
    layout->addWidget(m_trayOnlyRadio);
    layout->addWidget(m_KNotifyOnlyRadio);

    loadSettings();
}

NotifySettingsPage::~NotifySettingsPage()
{
}

void NotifySettingsPage::loadSettings()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);
    KConfigGroup notifyGroup(&notifierConfig, "Event");

    m_updatesCheckBox->setChecked(!notifyGroup.readEntry("hideUpdateNotifier", false));
    m_distUpgradeCheckBox->setChecked(!notifyGroup.readEntry("hideDistUpgradeNotifier", false));

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    QString notifyType = notifyTypeGroup.readEntry("NotifyType", "Combo");

    if (notifyType == "Combo") {
        m_comboRadio->setChecked(true);
    } else if (notifyType == "TrayOnly") {
        m_trayOnlyRadio->setChecked(true);
    } else {
        m_KNotifyOnlyRadio->setChecked(true);
    }
}

void NotifySettingsPage::applySettings()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);
    KConfigGroup notifyGroup(&notifierConfig, "Event");

    notifyGroup.writeEntry("hideUpdateNotifier", !m_updatesCheckBox->isChecked());
    notifyGroup.writeEntry("hideDistUpgradeNotifier", !m_distUpgradeCheckBox->isChecked());

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");

    notifyTypeGroup.writeEntry("NotifyType", m_comboRadio->isChecked() ? "Combo" :
                               m_trayOnlyRadio->isChecked() ? "TrayOnly" :
                               m_KNotifyOnlyRadio->isChecked() ? "KNotifyOnly" :
                               "Combo");
    notifyTypeGroup.sync();
}

void NotifySettingsPage::restoreDefaults()
{
    m_comboRadio->setChecked(true);
}

#include "NotifySettingsPage.moc"
