/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "DetailsTab.h"

#include <QtGui/QVBoxLayout>

DetailsTab::DetailsTab(QWidget *parent)
    : QWidget(parent)
    , m_backend(0)
    , m_package(0)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    setLayout(m_layout);
}

QString DetailsTab::name() const
{
    return m_name;
}

bool DetailsTab::shouldShow() const
{
    return true;
}

void DetailsTab::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void DetailsTab::setPackage(QApt::Package *package)
{
    m_package = package;
    refresh();
}

void DetailsTab::refresh()
{
}

void DetailsTab::clear()
{
    m_package = 0;
}
