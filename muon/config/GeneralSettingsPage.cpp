/***************************************************************************
 *   Copyright © 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include <QCheckBox>
#include <QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QSpinBox>
#include <QFormLayout>

#include <KDialog>
#include <KLocalizedString>

#include <LibQApt/Config>

#include "MuonSettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget* parent, QApt::Config *aptConfig)
        : SettingsPageBase(parent)
        , m_aptConfig(aptConfig)
        , m_askChangesCheckBox(new QCheckBox(this))
        , m_multiArchDupesBox(new QCheckBox(this))
        , m_recommendsCheckBox(new QCheckBox(this))
        , m_suggestsCheckBox(new QCheckBox(this))
        , m_untrustedCheckBox(new QCheckBox(this))
        , m_undoStackSpinbox(new QSpinBox(this))
        , m_autoCleanCheckBox(new QCheckBox(this))
        , m_autoCleanSpinbox(new QSpinBox(this))
{
    QFormLayout *layout = new QFormLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());
    setLayout(layout);

    m_askChangesCheckBox->setText(i18n("Ask to confirm changes that affect other packages"));
    m_multiArchDupesBox->setText(i18n("Show foreign-architecture packages that are available natively"));
    m_recommendsCheckBox->setText(i18n("Treat recommended packages as dependencies"));
    m_suggestsCheckBox->setText(i18n("Treat suggested packages as dependencies"));
    m_untrustedCheckBox->setText(i18n("Allow the installation of untrusted packages"));

    m_multiArchDupesBox->setEnabled(aptConfig->architectures().size() > 1);

    // Autoclean settings
    QWidget *autoCleanWidget = new QWidget(this);

    QHBoxLayout *autoCleanLayout = new QHBoxLayout(autoCleanWidget);
    autoCleanLayout->setMargin(0);
    autoCleanWidget->setLayout(autoCleanLayout);

    m_autoCleanCheckBox->setText(i18n("Delete obsolete cached packages every:"));
    m_autoCleanSpinbox->setMinimum(1);

    QWidget *autoCleanWidgetSpacer = new QWidget(autoCleanWidget);
    autoCleanWidgetSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    autoCleanLayout->addWidget(m_autoCleanCheckBox);
    autoCleanLayout->addWidget(m_autoCleanSpinbox);
    autoCleanLayout->addWidget(autoCleanWidgetSpacer);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    layout->addRow(m_askChangesCheckBox);
    layout->addRow(m_multiArchDupesBox);
    layout->addRow(m_recommendsCheckBox);
    layout->addRow(m_suggestsCheckBox);
    layout->addRow(m_untrustedCheckBox);
    layout->addRow(i18n("Number of undo operations:"), m_undoStackSpinbox);
    layout->addRow(autoCleanWidget);
    layout->addRow(spacer);

    connect(m_askChangesCheckBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_multiArchDupesBox, SIGNAL(clicked()), this, SIGNAL(changed()));
    connect(m_recommendsCheckBox, SIGNAL(clicked()), this, SLOT(emitAuthChanged()));
    connect(m_suggestsCheckBox, SIGNAL(clicked()), this, SLOT(emitAuthChanged()));
    connect(m_untrustedCheckBox, SIGNAL(clicked()), this, SLOT(emitAuthChanged()));
    connect(m_undoStackSpinbox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(m_autoCleanCheckBox, SIGNAL(clicked()), this, SLOT(emitAuthChanged()));
    connect(m_autoCleanSpinbox, SIGNAL(valueChanged(int)), this, SLOT(emitAuthChanged()));

    connect(m_autoCleanSpinbox, SIGNAL(valueChanged(int)),
            this, SLOT(updateAutoCleanSpinboxSuffix()));

    loadSettings();
    updateAutoCleanSpinboxSuffix();
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::loadSettings()
{
    MuonSettings *settings = MuonSettings::self();

    m_askChangesCheckBox->setChecked(settings->askChanges());
    m_multiArchDupesBox->setChecked(settings->showMultiArchDupes());
    m_recommendsCheckBox->setChecked(m_aptConfig->readEntry("APT::Install-Recommends", true));
    m_suggestsCheckBox->setChecked(m_aptConfig->readEntry("APT::Install-Suggests", false));
    m_untrustedCheckBox->setChecked(m_aptConfig->readEntry("APT::Get::AllowUnauthenticated", false));
    m_undoStackSpinbox->setValue(settings->undoStackSize());

    int autoCleanValue = m_aptConfig->readEntry("APT::Periodic::AutocleanInterval", 0);
    m_autoCleanCheckBox->setChecked(autoCleanValue > 0);
    m_autoCleanSpinbox->setValue(autoCleanValue);
}

void GeneralSettingsPage::applySettings()
{
    MuonSettings *settings = MuonSettings::self();

    settings->setAskChanges(m_askChangesCheckBox->isChecked());
    settings->setShowMultiArchDupes(m_multiArchDupesBox->isChecked());
    settings->setUndoStackSize(m_undoStackSpinbox->value());
    settings->writeConfig();

    // Only write if changed. Unnecessary password dialogs ftl
    if (m_aptConfig->readEntry("APT::Install-Recommends", true) != m_recommendsCheckBox->isChecked()) {
        m_aptConfig->writeEntry("APT::Install-Recommends", m_recommendsCheckBox->isChecked());
    }

    if (m_aptConfig->readEntry("APT::Install-Suggests", false) != m_suggestsCheckBox->isChecked()) {
        m_aptConfig->writeEntry("APT::Install-Suggests", m_suggestsCheckBox->isChecked());
    }

    if (m_aptConfig->readEntry("APT::Get::AllowUnauthenticated", true) != m_untrustedCheckBox->isChecked()) {
        m_aptConfig->writeEntry("APT::Get::AllowUnauthenticated", m_untrustedCheckBox->isChecked());
    }

    int aCleanValue = autoCleanValue();

    if (m_aptConfig->readEntry("APT::Periodic::AutocleanInterval", 0) != aCleanValue) {
        m_aptConfig->writeEntry("APT::Periodic::AutocleanInterval", aCleanValue);
    }
}

void GeneralSettingsPage::restoreDefaults()
{
    m_undoStackSpinbox->setValue(20);
}

void GeneralSettingsPage::updateAutoCleanSpinboxSuffix()
{
    m_autoCleanSpinbox->setSuffix(i18np(" day", " days", m_autoCleanSpinbox->value()));
}

int GeneralSettingsPage::autoCleanValue() const
{
    int autoCleanValue;

    if (m_autoCleanCheckBox->isChecked()) {
        autoCleanValue = m_autoCleanSpinbox->value();
    } else {
        autoCleanValue = 0;
    }

    return autoCleanValue;
}

void GeneralSettingsPage::emitAuthChanged()
{
    bool recChanged = m_aptConfig->readEntry("APT::Install-Recommends", true)
                      != m_recommendsCheckBox->isChecked();
    bool sugChanged = m_aptConfig->readEntry("APT::Install-Suggests", false)
                      != m_suggestsCheckBox->isChecked();
    bool trustChanged = m_aptConfig->readEntry("APT::Get::AllowUnauthenticated", true)
                        != m_untrustedCheckBox->isChecked();

    int cleanInt = autoCleanValue();
    bool cleanIntChanged = m_aptConfig->readEntry("APT::Periodic::AutocleanInterval", 0) != cleanInt;

    if (recChanged || sugChanged || cleanIntChanged || trustChanged) {
        emit authChanged();
    } else {
        emit changed();
    }
}

#include "GeneralSettingsPage.moc"
