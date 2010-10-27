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

#include "GeneralSettingsPage.h"

#include <QtGui/QCheckBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>

#include <KDialog>
#include <KLocale>

#include <LibQApt/Config>

#include "MuonSettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget* parent, QApt::Config *aptConfig) :
        SettingsPageBase(parent)
        , m_aptConfig(aptConfig)
        , m_recommendsCheckBox(new QCheckBox(this))
        , m_undoStackSpinbox(new QSpinBox(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());
    setLayout(layout);

    m_recommendsCheckBox->setText(i18n("Treat recommended packages as dependencies"));

    QWidget *undoSizeWidget = new QWidget(this);

    QHBoxLayout *undoSizeLayout = new QHBoxLayout(undoSizeWidget);
    undoSizeWidget->setLayout(undoSizeLayout);

    QLabel *undoLabel = new QLabel(this);
    undoLabel->setText(i18n("Number of undo operations:"));

    QWidget *undoSizeWidgetSpacer = new QWidget(undoSizeWidget);
    undoSizeWidgetSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    undoSizeLayout->addWidget(undoLabel);
    undoSizeLayout->addWidget(m_undoStackSpinbox);
    undoSizeLayout->addWidget(undoSizeWidgetSpacer);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    layout->addWidget(m_recommendsCheckBox);
    layout->addWidget(undoSizeWidget);
    layout->addWidget(spacer);

    connect(m_recommendsCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_undoStackSpinbox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));

    loadSettings();
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::loadSettings()
{
    MuonSettings *settings = MuonSettings::self();

    m_recommendsCheckBox->setChecked(m_aptConfig->readEntry("APT::Install-Recommends", true));
    m_undoStackSpinbox->setValue(settings->undoStackSize());
}

void GeneralSettingsPage::applySettings()
{
    MuonSettings *settings = MuonSettings::self();

    settings->setUndoStackSize(m_undoStackSpinbox->value());
    settings->writeConfig();

    // Only write if changed. Unnecessary password dialogs ftl
    if (m_aptConfig->readEntry("APT::Install-Recommends", false) != m_recommendsCheckBox->isChecked()) {
        // TODO: Change apply button icon to the auth key.
        m_aptConfig->writeEntry("APT::Install-Recommends", m_recommendsCheckBox->isChecked());
    }
}

void GeneralSettingsPage::restoreDefaults()
{
    m_undoStackSpinbox->setValue(20);
}

#include "GeneralSettingsPage.moc"
