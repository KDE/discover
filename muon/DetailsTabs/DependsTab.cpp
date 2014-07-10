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

#include "DependsTab.h"

// Qt includes
#include <QScrollBar>

// KDE includes
#include <KComboBox>
#include <KLocale>
#include <KTextBrowser>

// LibQApt includes
#include <LibQApt/Package>

DependsTab::DependsTab(QWidget *parent)
    : DetailsTab(parent)
{
    m_name = i18nc("@title:tab", "Dependencies");

    m_comboBox = new KComboBox(this);
    m_comboBox->addItem(i18nc("@item:inlistbox", "Dependencies of the Current Version"), CurrentVersionType);
    m_comboBox->addItem(i18nc("@item:inlistbox", "Dependencies of the Latest Version"),  LatestVersionType);
    m_comboBox->addItem(i18nc("@item:inlistbox", "Dependants (Reverse Dependencies)"), ReverseDependsType);
    m_comboBox->addItem(i18nc("@item:inlistbox", "Virtual Packages Provided"), VirtualDependsType);
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(populateDepends(int)));
    m_dependsBrowser = new KTextBrowser(this);

    m_layout->addWidget(m_comboBox);
    m_layout->addWidget(m_dependsBrowser);
}

void DependsTab::refresh()
{
    if (!m_package) {
        return; // Nothing to refresh yet, so return, else we crash
    }

    m_dependsBrowser->setText(QString());
    populateDepends(m_comboBox->currentIndex());
}

void DependsTab::populateDepends(int index)
{
    m_dependsBrowser->setText(QString());
    QStringList list;

    int depType = m_comboBox->itemData(index).toInt();
    switch (depType) {
    case CurrentVersionType:
        list = m_package->dependencyList(false);
        if (list.isEmpty()) {
            m_dependsBrowser->append(i18nc("@label", "This package does not have any dependencies"));
            return;
        }
        break;
    case LatestVersionType:
        list = m_package->dependencyList(true);
        if (list.isEmpty()) {
            m_dependsBrowser->append(i18nc("@label", "This package does not have any dependencies"));
            return;
        }
        break;
    case ReverseDependsType:
        list = m_package->requiredByList();
        if (list.isEmpty()) {
            m_dependsBrowser->append(i18nc("@label", "This package has no dependents. (Nothing depends on it.)"));
            return;
        }
        break;
    case VirtualDependsType:
        list = m_package->providesList();
        if (list.isEmpty()) {
            m_dependsBrowser->append(i18nc("@label", "This package does not provide any virtual packages"));
            return;
        }
        break;
    }

    foreach(const QString &string, list) {
            m_dependsBrowser->append(string);
    }

    QTextCursor cursor;
    cursor.movePosition(QTextCursor::Start);
    m_dependsBrowser->setTextCursor(cursor);
}

#include "DependsTab.moc"
