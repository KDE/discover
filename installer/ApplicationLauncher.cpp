/***************************************************************************
 *   Copyright © 2010 by Daniel Nicoletti <dantti85-pk@yahoo.com.br>       *
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include "ApplicationLauncher.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItemModel>
#include <QtGui/QVBoxLayout>

#include <KConfigGroup>
#include <KIcon>
#include <KLocale>
#include <KService>
#include <KStandardGuiItem>
#include <KToolInvocation>

ApplicationLauncher::ApplicationLauncher(const QVector<KService*> &applications, QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QLabel *label = new QLabel(this);
    label->setText(i18np("The following application was just installed, click on it to launch:",
                         "The following applications were just installed, click on them to launch:",
                         applications.size()));

    QListView *appView = new QListView(this);
    appView->setIconSize(QSize(32, 32));
    connect(appView, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(onAppClicked(const QModelIndex &)));

    QWidget *bottomBox = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBox);
    bottomBox->setLayout(bottomLayout);

    m_noShowCheckBox = new QCheckBox(bottomBox);
    m_noShowCheckBox->setText(i18n("Don't show this dialog again"));

    QWidget *bottomSpacer = new QWidget(bottomBox);
    bottomSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QPushButton *closeButton = new QPushButton(bottomBox);
    KGuiItem closeItem = KStandardGuiItem::close();
    closeButton->setText(closeItem.text());
    closeButton->setIcon(closeItem.icon());
    connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));

    bottomLayout->addWidget(m_noShowCheckBox);
    bottomLayout->addWidget(bottomSpacer);
    bottomLayout->addWidget(closeButton);

    m_model = new QStandardItemModel(this);
    addApplications(applications);
    appView->setModel(m_model);

    layout->addWidget(label);
    layout->addWidget(appView);
    layout->addWidget(bottomBox);
}

ApplicationLauncher::~ApplicationLauncher()
{
    if (m_noShowCheckBox->isChecked()) {
        KConfig config("muon-installerrc");
        KConfigGroup notifyGroup(&config, "Notification Messages");
        notifyGroup.writeEntry("ShowApplicationLauncher", false);
    }
}

void ApplicationLauncher::addApplications(const QVector<KService*> &applications)
{
    QStandardItem *item = 0;
    foreach (const KService *service, applications) {
        QString name = service->genericName().isEmpty() ?
                       service->property("Name").toString() :
                       service->property("Name").toString() % QLatin1Literal(" - ") % service->genericName();
        item = new QStandardItem(name);
        item->setIcon(KIcon(service->icon()));
        item->setData(service->desktopEntryPath(), Qt::UserRole);
        m_model->appendRow(item);
    }
}

void ApplicationLauncher::onAppClicked(const QModelIndex &index)
{
    KToolInvocation::startServiceByDesktopPath(index.data(Qt::UserRole).toString());
}

#include "ApplicationLauncher.moc"
