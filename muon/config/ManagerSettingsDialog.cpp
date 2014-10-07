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

#include <QPushButton>
#include <KLocalizedString>

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
    setWindowTitle(i18nc("@title:window", "Muon Preferences"));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    button(QDialogButtonBox::Apply)->setEnabled(false);

    // General settings
    GeneralSettingsPage *generalPage = new GeneralSettingsPage(this, m_aptConfig);
    KPageWidgetItem *generalSettingsFrame = addPage(generalPage,
                                                    i18nc("@title:group Title of the general group", "General"));
    generalSettingsFrame->setIcon(QIcon::fromTheme("system-run"));
    connect(generalPage, SIGNAL(changed()), this, SLOT(changed()));
    connect(generalPage, SIGNAL(authChanged()), this, SLOT(authChanged()));

    // Notification settings
    NotifySettingsPage *notifyPage = new NotifySettingsPage(this);
    KPageWidgetItem *notifySettingsFrame = addPage(notifyPage,
                                                    i18nc("@title:group", "Notifications"));
    notifySettingsFrame->setIcon(QIcon::fromTheme("preferences-desktop-notification"));
    connect(notifyPage, SIGNAL(changed()), this, SLOT(changed()));

    m_pages.insert(generalPage);
    m_pages.insert(notifyPage);
}

ManagerSettingsDialog::~ManagerSettingsDialog()
{
}

void ManagerSettingsDialog::slotButtonClicked(QAbstractButton* b)
{
    if ((b == button(QDialogButtonBox::Ok)) || (b == button(QDialogButtonBox::Apply))) {
        applySettings();
    } else if (b == button(QDialogButtonBox::RestoreDefaults)) {
        restoreDefaults();
    }
}

void ManagerSettingsDialog::changed()
{
    button(QDialogButtonBox::Apply)->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    button(QDialogButtonBox::Apply)->setEnabled(true);
}

void ManagerSettingsDialog::authChanged()
{
    button(QDialogButtonBox::Apply)->setIcon(QIcon::fromTheme("dialog-password"));
    button(QDialogButtonBox::Apply)->setEnabled(true);
}

void ManagerSettingsDialog::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }

    emit settingsChanged();
    button(QDialogButtonBox::Apply)->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    button(QDialogButtonBox::Apply)->setEnabled(false);
}

void ManagerSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }

    button(QDialogButtonBox::Apply)->setIcon(QIcon::fromTheme("dialog-ok-apply"));
}

#include "ManagerSettingsDialog.moc"
