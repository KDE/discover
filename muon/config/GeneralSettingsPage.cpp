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

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>

#include <KDialog>
#include <KLocale>

#include "MuonSettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget* parent) :
        SettingsPageBase(parent)
        , m_undoStackSpinbox(new QSpinBox(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());
    setLayout(layout);

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

    layout->addWidget(undoSizeWidget);

    connect(m_undoStackSpinbox, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));

    loadSettings();
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::loadSettings()
{
    MuonSettings *settings = MuonSettings::self();

    m_undoStackSpinbox->setValue(settings->undoStackSize());
}

void GeneralSettingsPage::applySettings()
{
    MuonSettings *settings = MuonSettings::self();

    settings->setUndoStackSize(m_undoStackSpinbox->value());
    settings->writeConfig();
}

void GeneralSettingsPage::restoreDefaults()
{
    m_undoStackSpinbox->setValue(20);
}

#include "GeneralSettingsPage.moc"
