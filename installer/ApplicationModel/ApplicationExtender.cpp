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

#include "ApplicationExtender.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

#include <KIcon>
#include <KLocale>

#include <LibQApt/Package>

#include "Application.h"

ApplicationExtender::ApplicationExtender(QWidget *parent, Application *app)
    : QWidget(parent)
    , m_app(app)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    QPushButton *infoButton = new QPushButton(this);
    infoButton->setText(i18n("More Info"));
    layout->addWidget(infoButton);
    connect(infoButton, SIGNAL(clicked()), this, SLOT(emitInfoButtonClicked()));

    QWidget *buttonSpacer = new QWidget(this);
    buttonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    layout->addWidget(buttonSpacer);

    QPushButton *actionButton = new QPushButton(this);

    if (app->package()->isInstalled()) {
        actionButton->setIcon(KIcon("edit-delete"));
        actionButton->setText(i18n("Remove"));
        connect(actionButton, SIGNAL(clicked()), this, SLOT(emitRemoveButtonClicked()));
    } else {
        actionButton->setIcon(KIcon("download"));
        actionButton->setText(i18n("Install"));
        connect(actionButton, SIGNAL(clicked()), this, SLOT(emitInstallButtonClicked()));
    }
    layout->addWidget(actionButton);
}

ApplicationExtender::~ApplicationExtender()
{
}

void ApplicationExtender::emitInfoButtonClicked()
{
    emit infoButtonClicked(m_app);
}

void ApplicationExtender::emitRemoveButtonClicked()
{
    emit removeButtonClicked(m_app);
}

void ApplicationExtender::emitInstallButtonClicked()
{
    emit installButtonClicked(m_app);
}

#include "ApplicationExtender.moc"
