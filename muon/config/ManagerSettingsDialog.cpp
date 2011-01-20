/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2010 by Jonathan Thomas <echidnaman@kubuntu.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "ManagerSettingsDialog.h"

#include <KIcon>
#include <KLocale>

#include <LibQApt/Config>

#include "../../libmuon/settings/SettingsPageBase.h"
#include "../../libmuon/settings/NotifySettingsPage.h"
#include "GeneralSettingsPage.h"

ManagerSettingsDialog::ManagerSettingsDialog(QWidget* parent, QApt::Config *aptConfig) :
    KPageDialog(parent),
    m_aptConfig(aptConfig)

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setCaption(i18nc("@title:window", "Muon Preferences"));
    setButtons(Ok | Apply | Cancel | Default);
    enableButtonApply(false);
    setDefaultButton(Ok);

    // General settings
    GeneralSettingsPage *generalPage = new GeneralSettingsPage(this, m_aptConfig);
    KPageWidgetItem *generalSettingsFrame = addPage(generalPage,
                                                    i18nc("@title:group Title of the general group", "General"));
    generalSettingsFrame->setIcon(KIcon("system-run"));
    connect(generalPage, SIGNAL(changed()), this, SLOT(changed()));
    connect(generalPage, SIGNAL(authChanged()), this, SLOT(authChanged()));

    // Notification settings
    NotifySettingsPage *notifyPage = new NotifySettingsPage(this);
    KPageWidgetItem *notifySettingsFrame = addPage(notifyPage,
                                                    i18nc("@title:group", "Notifications"));
    notifySettingsFrame->setIcon(KIcon("preferences-desktop-notification"));
    connect(notifyPage, SIGNAL(changed()), this, SLOT(changed()));

    m_pages.insert(generalPage);
    m_pages.insert(notifyPage);
}

ManagerSettingsDialog::~ManagerSettingsDialog()
{
}

void ManagerSettingsDialog::slotButtonClicked(int button)
{
    if ((button == Ok) || (button == Apply)) {
        applySettings();
    } else if (button == Default) {
        restoreDefaults();
    }

    KPageDialog::slotButtonClicked(button);
}

void ManagerSettingsDialog::changed()
{
    setButtonIcon(Apply, KIcon("dialog-ok-apply"));

    enableButtonApply(true);
}

void ManagerSettingsDialog::authChanged()
{
    setButtonIcon(Apply, KIcon("dialog-password"));
    enableButtonApply(true);
}

void ManagerSettingsDialog::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }

    emit settingsChanged();
    setButtonIcon(Apply, KIcon("dialog-ok-apply"));
    enableButtonApply(false);
}

void ManagerSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }

    setButtonIcon(Apply, KIcon("dialog-ok-apply"));
}

#include "ManagerSettingsDialog.moc"
