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

#include "ManagerWidget.h"

// Qt includes
#include <QtCore/QTimer>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSplitter>
#include <QtGui/QTreeView>

// KDE includes
#include <KDebug>
#include <KIcon>
#include <KLineEdit>
#include <KLocale>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "DetailsWidget.h"
#include "PackageModel/PackageModel.h"
#include "PackageModel/PackageProxyModel.h"
#include "PackageModel/PackageDelegate.h"

ManagerWidget::ManagerWidget(QWidget *parent, QApt::Backend *backend)
    : KVBox(parent)
    , m_backend(backend)
{
    m_model = new PackageModel(this);
    PackageDelegate *delegate = new PackageDelegate(this);

    m_proxyModel = new PackageProxyModel(this, m_backend);
    m_proxyModel->setSourceModel(m_model);

    KVBox *topVBox = new KVBox;

    QLabel *browserHeader = new QLabel(this);
    browserHeader->setTextFormat(Qt::RichText);
    browserHeader->setText(i18n("<b>Browse Packages</b>"));

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(400);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));

    m_searchEdit = new KLineEdit(topVBox);
    m_searchEdit->setClickMessage("Search for packages");
    m_searchEdit->setClearButtonShown(true);
    connect(m_searchEdit, SIGNAL(textChanged(const QString &)), m_searchTimer, SLOT(start()));

    m_packageView = new QTreeView(topVBox);
    m_packageView->setModel(m_proxyModel);
    m_packageView->setItemDelegate(delegate);
    m_packageView->setAlternatingRowColors(true);
    m_packageView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_packageView->setRootIsDecorated(false);
    connect (m_packageView, SIGNAL(activated(const QModelIndex&)),
             this, SLOT(packageActivated(const QModelIndex&)));

    QApt::PackageList list = m_backend->availablePackages();
    foreach (QApt::Package *package, list) {
        m_model->addPackage(package);
    }

    KVBox *bottomVBox = new KVBox;

    m_detailsWidget = new DetailsWidget(bottomVBox);

    QWidget *hbox = new QWidget(bottomVBox);
    QHBoxLayout *layout = new QHBoxLayout(hbox);
    hbox->setLayout(layout);
    layout->addStretch();

    QPushButton *revertButton = new QPushButton(hbox);
    revertButton->setIcon(KIcon("document-revert"));
    revertButton->setText(i18n("Revert Changes"));
    revertButton->setEnabled(false);
    layout->addWidget(revertButton);

    QPushButton *applyButton = new QPushButton(hbox);
    applyButton->setIcon(KIcon("dialog-ok-apply"));
    applyButton->setText(i18n("Review Changes"));
    applyButton->setEnabled(false);
    layout->addWidget(applyButton);

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(topVBox);
    splitter->addWidget(bottomVBox);
}

ManagerWidget::~ManagerWidget()
{
}

void ManagerWidget::packageActivated(const QModelIndex &index)
{
    QApt::Package *package = m_proxyModel->packageAt(index);
    m_detailsWidget->setPackage(package);
}

void ManagerWidget::startSearch()
{
    // 1-character searches are painfully slow
    if (m_searchEdit->text().size() > 1) {
        m_proxyModel->search(m_searchEdit->text());
    }
}

#include "ManagerWidget.moc"
