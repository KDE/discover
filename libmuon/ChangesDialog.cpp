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

#include "ChangesDialog.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KStandardGuiItem>

// Own includes
#include "../libmuon/MuonStrings.h"

ChangesDialog::ChangesDialog(QWidget *parent, const QApt::StateChanges &changes)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QLabel *headerLabel = new QLabel(this);
    headerLabel->setText(i18nc("@info", "<h2>Mark additional changes?</h2>"));

    QLabel *label = new QLabel(this);
    label->setText(i18np("This action requires a change to another package:",
                         "This action requires changes to other packages:",
                         changes.size()));

    QTreeView *packageView = new QTreeView(this);
    packageView->setHeaderHidden(true);
    packageView->setRootIsDecorated(false);

    QWidget *bottomBox = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBox);
    bottomLayout->setSpacing(0);
    bottomLayout->setMargin(0);
    bottomBox->setLayout(bottomLayout);

    QWidget *bottomSpacer = new QWidget(bottomBox);
    bottomSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QPushButton *okButton = new QPushButton(bottomBox);
    KGuiItem okItem = KStandardGuiItem::ok();
    okButton->setText(okItem.text());
    okButton->setIcon(okItem.icon());
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *cancelButton = new QPushButton(bottomBox);
    KGuiItem cancelItem = KStandardGuiItem::cancel();
    cancelButton->setText(cancelItem.text());
    cancelButton->setIcon(cancelItem.icon());
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    bottomLayout->addWidget(bottomSpacer);
    bottomLayout->addWidget(okButton);
    bottomLayout->addWidget(cancelButton);

    m_model = new QStandardItemModel(this);
    packageView->setModel(m_model);
    addPackages(changes);
    packageView->expandAll();

    layout->addWidget(headerLabel);
    layout->addWidget(label);
    layout->addWidget(packageView);
    layout->addWidget(bottomBox);
}

void ChangesDialog::addPackages(const QApt::StateChanges &changes)
{
    for (auto i = changes.constBegin(); i != changes.constEnd(); ++i) {
        QStandardItem *root = new QStandardItem;
        root->setText(MuonStrings::global()->packageStateName(i.key()));
        root->setEditable(false);

        QFont font = root->font();
        font.setBold(true);
        root->setFont(font);

        const QApt::PackageList packages = i.value();

        QStandardItem *item = 0;
        for (QApt::Package *package : packages) {
            item = new QStandardItem;
            item->setText(package->name());
            item->setEditable(false);
            item->setIcon(KIcon("muon"));

            root->appendRow(item);
        }

        m_model->appendRow(root);
    }
}
