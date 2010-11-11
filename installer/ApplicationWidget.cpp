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

#include "ApplicationWidget.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

#include <KIcon>
#include <KLocale>

#include <LibQApt/Package>

#include "Application.h"

ApplicationWidget::ApplicationWidget(QWidget *parent, Application *app)
    : QWidget(parent)
    , m_app(app)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    // Header
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerWidget->setLayout(headerLayout);

    m_iconLabel = new QLabel(headerWidget);
    // FIXME: Bigger if possible
    m_iconLabel->setPixmap(KIcon(app->icon()).pixmap(32,32));

    QWidget *nameDescWidget = new QWidget(headerWidget);
    QVBoxLayout *nameDescLayout = new QVBoxLayout(nameDescWidget);
    m_nameLabel = new QLabel(nameDescWidget);
    m_nameLabel->setText(QLatin1Literal("<h1>") % app->name() % QLatin1Literal("</h1>"));
    m_descriptionLabel = new QLabel(nameDescWidget);
    m_descriptionLabel->setText(app->comment());

    nameDescLayout->addWidget(m_nameLabel);
    nameDescLayout->addWidget(m_descriptionLabel);

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(nameDescWidget);

    QWidget *verticalSpacer = new QWidget(this);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);


    layout->addWidget(headerWidget);
    layout->addWidget(verticalSpacer);
}

ApplicationWidget::~ApplicationWidget()
{
}

#include "ApplicationWidget.moc"


