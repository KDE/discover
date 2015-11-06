/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ArchitectureFilter.h"

// KDE includes
#include <KLocalizedString>

// QApt includes
#include <QApt/Backend>

// Libmuon includes
#include "muonapt/MuonStrings.h"

ArchitectureFilter::ArchitectureFilter(QObject *parent, QApt::Backend *backend)
    : FilterModel(parent)
    , m_backend(backend)
    , m_shouldShow(true)
{
}

void ArchitectureFilter::populate()
{
    QStringList archList = m_backend->architectures();
    archList.prepend("all");

    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(QIcon::fromTheme("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));
    appendRow(defaultItem);

    Q_FOREACH (const QString &arch, archList) {
        QStandardItem *archItem = new QStandardItem;
        archItem->setEditable(false);
        archItem->setText(MuonStrings::global()->archString(arch));
        archItem->setData(arch, Qt::UserRole+1);
        appendRow(archItem);
    }
}
