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
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

// KDE includes
#include <KLocalizedString>
#include <KStandardGuiItem>

// Own includes
#include "muonapt/MuonStrings.h"

ChangesDialog::ChangesDialog(QWidget *parent, const QApt::StateChanges &changes)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Confirm Additional Changes"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QLabel *headerLabel = new QLabel(this);
    headerLabel->setText(i18nc("@info", "<h2>Mark additional changes?</h2>"));

    int count = countChanges(changes);
    QLabel *label = new QLabel(this);
    label->setText(i18np("This action requires a change to another package:",
                         "This action requires changes to other packages:",
                         count));

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
    packageView->setEditTriggers(QAbstractItemView::NoEditTriggers);

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

        QFont font = root->font();
        font.setBold(true);
        root->setFont(font);

        Q_FOREACH (QApt::Package *package, *i) {
            root->appendRow(new QStandardItem(QIcon::fromTheme("muon"), package->name()));
        }

        m_model->appendRow(root);
    }
}

int ChangesDialog::countChanges(const QApt::StateChanges &changes)
{
    int count = 0;
    foreach (const QApt::PackageList& pkgs, changes) {
        count += pkgs.size();
    }
    return count;
}
