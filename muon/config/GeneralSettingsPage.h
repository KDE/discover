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

#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include <QtWidgets/QWidget>

#include "../settings/SettingsPageBase.h"

class QCheckBox;
class QSpinBox;

namespace QApt {
    class Config;
}

class GeneralSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    GeneralSettingsPage(QWidget* parent, QApt::Config *aptConfig);
    virtual ~GeneralSettingsPage();

    void loadSettings();
    virtual void applySettings();
    virtual void restoreDefaults();

private Q_SLOTS:
    void emitAuthChanged();
    void updateAutoCleanSpinboxSuffix();

private:
    QApt::Config *m_aptConfig;
    QCheckBox *m_askChangesCheckBox;
    QCheckBox *m_multiArchDupesBox;
    QCheckBox *m_recommendsCheckBox;
    QCheckBox *m_suggestsCheckBox;
    QCheckBox *m_untrustedCheckBox;
    QSpinBox *m_undoStackSpinbox;
    QCheckBox *m_autoCleanCheckBox;
    QSpinBox *m_autoCleanSpinbox;

    int autoCleanValue() const;
};

#endif
