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

#include "UpdaterSettingsDialog.h"

#include "../../libmuon/settings/SettingsPageBase.h"
#include "../../libmuon/settings/NotifySettingsPage.h"
#include <klocalizedstring.h>
#include <QIcon>
#include <QDialogButtonBox>
#include <QPushButton>

UpdaterSettingsDialog::UpdaterSettingsDialog(QWidget* parent) :
    KPageDialog(parent),
    m_pages()

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setWindowTitle(i18nc("@title:window", "Muon Preferences"));
    buttonBox()->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
    connect(buttonBox(), SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox(), SIGNAL(rejected()), SLOT(reject()));
    connect(this, SIGNAL(accepted()), SLOT(applySettings()));
    connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked(bool)), SLOT(restoreDefaults()));

    // Notification settings
    NotifySettingsPage *notifyPage = new NotifySettingsPage(this);
    KPageWidgetItem* notifySettingsFrame = addPage(notifyPage,
                                                    i18nc("@title:group", "Notifications"));
    notifySettingsFrame->setIcon(QIcon::fromTheme("preferences-desktop-notification"));
    connect(notifyPage, SIGNAL(changed()), this, SLOT(enableApply()));

    m_pages.append(notifyPage);
}

UpdaterSettingsDialog::~UpdaterSettingsDialog()
{
}

void UpdaterSettingsDialog::enableApply()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void UpdaterSettingsDialog::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }

    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void UpdaterSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
}

#include "UpdaterSettingsDialog.moc"
