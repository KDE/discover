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

#include "ApplicationDetailsView.h"

// Qt includes
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KIcon>

// Own includes
#include "ApplicationDetailsWidget.h"
#include "../Application.h"
#include "../BreadcrumbWidget/BreadcrumbItem.h"

ApplicationDetailsView::ApplicationDetailsView(QWidget *parent, Application *app)
    : AbstractViewBase(parent)
{
    m_detailsWidget = new ApplicationDetailsWidget(this);
    m_detailsWidget->setApplication(app);

    m_layout->addWidget(m_detailsWidget);

    m_crumb->setText(app->name());
    m_crumb->setIcon(KIcon(app->icon()));
    m_crumb->setAssociatedView(this);
}

ApplicationDetailsView::~ApplicationDetailsView()
{
}

#include "ApplicationDetailsView.moc"
